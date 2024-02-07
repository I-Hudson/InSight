#include "Graphics/RHI/RHI_GPUCrashTracker.h"
#include "Core/Memory.h"
#include "FileSystem/FileSystem.h"

#include <string>

namespace Insight
{
	namespace Graphics
	{
		constexpr static const char* c_CrashFolder = "gpuCrashDumps/";
		constexpr static const char* c_shaderFolder = "gpuShaders/";

		RHI_GPUCrashTracker* RHI_GPUCrashTracker::Create()
		{
#ifdef IS_NVIDIA_AFTERMATH_ENABLED
			return NewTracked(RHI_GPUCrashTrackerNvidiaAftermath);
#endif // IS_NVIDIA_AFTERMATH_ENABLED
			return nullptr;
		}

#ifdef IS_NVIDIA_AFTERMATH_ENABLED
		void RHI_GPUCrashTrackerNvidiaAftermath::Init()
		{
			// Enable GPU crash dumps and set up the callbacks for crash dump notifications,
			// shader debug information notifications, and providing additional crash
			// dump description data.Only the crash dump callback is mandatory. The other two
			// callbacks are optional and can be omitted, by passing nullptr, if the corresponding
			// functionality is not used.
			// The DeferDebugInfoCallbacks flag enables caching of shader debug information data
			// in memory. If the flag is set, ShaderDebugInfoCallback will be called only
			// in the event of a crash, right before GpuCrashDumpCallback. If the flag is not set,
			// ShaderDebugInfoCallback will be called for every shader that is compiled.
			AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_EnableGpuCrashDumps(
				GFSDK_Aftermath_Version_API,
				GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_Vulkan | GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_DX,
				GFSDK_Aftermath_GpuCrashDumpFeatureFlags_Default,				  // Let the Nsight Aftermath library cache shader debug information.
				GpuCrashDumpCallback,                                             // Register callback for GPU crash dumps.
				ShaderDebugInfoCallback,                                          // Register callback for shader debug information.
				CrashDumpDescriptionCallback,                                     // Register callback for GPU crash dump description.
				ResolveMarkerCallback,                                            // Register callback for resolving application-managed markers.
				this));                                                           // Set the GpuCrashTracker object as user data for the above callbacks.
		}

		void RHI_GPUCrashTrackerNvidiaAftermath::Destroy()
		{
			AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_DisableGpuCrashDumps());
		}

		void RHI_GPUCrashTrackerNvidiaAftermath::OnCrashDump(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize)
		{
			// Make sure only one thread at a time...
			std::lock_guard<std::mutex> lock(m_mutex);
			// Write to file for later in-depth analysis with Nsight Graphics.
			WriteGpuCrashDumpToFile(pGpuCrashDump, gpuCrashDumpSize);
		}

		void RHI_GPUCrashTrackerNvidiaAftermath::OnShaderDebugInfo(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize)
		{
			// Make sure only one thread at a time...
			std::lock_guard<std::mutex> lock(m_mutex);

			// Get shader debug information identifier
			GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier = {};
			AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GetShaderDebugInfoIdentifier(
				GFSDK_Aftermath_Version_API,
				pShaderDebugInfo,
				shaderDebugInfoSize,
				&identifier));

			// Store information for decoding of GPU crash dumps with shader address mapping
			// from within the application.
			std::vector<uint8_t> data((uint8_t*)pShaderDebugInfo, (uint8_t*)pShaderDebugInfo + shaderDebugInfoSize);
			//m_shaderDebugInfo[identifier].swap(data);

			// Write to file for later in-depth analysis of crash dumps with Nsight Graphics
			WriteShaderDebugInformationToFile(identifier, pShaderDebugInfo, shaderDebugInfoSize);
		}

		void RHI_GPUCrashTrackerNvidiaAftermath::OnDescription(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription)
		{
			// Add some basic description about the crash. This is called after the GPU crash happens, but before
			// the actual GPU crash dump callback. The provided data is included in the crash dump and can be
			// retrieved using GFSDK_Aftermath_GpuCrashDump_GetDescription().
			addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName, "Insight");
			addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationVersion, "v1.0");
			addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_UserDefined, "This is a GPU crash dump example.");
			addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_UserDefined + 1, "Engine State: Rendering.");
			addDescription(GFSDK_Aftermath_GpuCrashDumpDescriptionKey_UserDefined + 2, "More user-defined information...");
		}

		void RHI_GPUCrashTrackerNvidiaAftermath::OnResolveMarker(const void* pMarker, void** resolvedMarkerData, uint32_t* markerSize)
		{
			// Important: the pointer passed back via resolvedMarkerData must remain valid after this function returns
			// using references for all of the m_markerMap accesses ensures that the pointers refer to the persistent data
			//for (auto& map : m_markerMap)
			//{
			//	const auto& foundMarker = map.find((uint64_t)pMarker);
			//	if (foundMarker != map.end())
			//	{
			//		const std::string& markerData = foundMarker->second;
			//		// std::string::data() will return a valid pointer until the string is next modified
			//		// we don't modify the string after calling data() here, so the pointer should remain valid
			//		*resolvedMarkerData = (void*)markerData.data();
			//		*markerSize = (uint32_t)markerData.length();
			//		return;
			//	}
			//}
		}

		void RHI_GPUCrashTrackerNvidiaAftermath::WriteGpuCrashDumpToFile(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize)
		{
			// Create a GPU crash dump decoder object for the GPU crash dump.
			GFSDK_Aftermath_GpuCrashDump_Decoder decoder = {};
			AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_CreateDecoder(
				GFSDK_Aftermath_Version_API,
				pGpuCrashDump,
				gpuCrashDumpSize,
				&decoder));

			// Use the decoder object to read basic information, like application
			// name, PID, etc. from the GPU crash dump.
			GFSDK_Aftermath_GpuCrashDump_BaseInfo baseInfo = {};
			AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_GetBaseInfo(decoder, &baseInfo));

			// Use the decoder object to query the application name that was set
			// in the GPU crash dump description.
			uint32_t applicationNameLength = 0;
			AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_GetDescriptionSize(
				decoder,
				GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName,
				&applicationNameLength));

			std::vector<char> applicationName(applicationNameLength, '\0');

			AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_GetDescription(
				decoder,
				GFSDK_Aftermath_GpuCrashDumpDescriptionKey_ApplicationName,
				uint32_t(applicationName.size()),
				applicationName.data()));

			// Create a unique file name for writing the crash dump data to a file.
			// Note: due to an Nsight Aftermath bug (will be fixed in an upcoming
			// driver release) we may see redundant crash dumps. As a workaround,
			// attach a unique count to each generated file name.
			static int count = 0;
			const std::string baseFileName =
				std::string(applicationName.data())
				+ "-"
				+ std::to_string(baseInfo.pid)
				+ "-"
				+ std::to_string(++count);

			// Write the crash dump data to a file using the .nv-gpudmp extension
			// registered with Nsight Graphics.
			const std::string crashDumpFileName = c_CrashFolder + baseFileName + ".nv-gpudmp";
			FileSystem::CreateFolder(c_CrashFolder);

			std::ofstream dumpFile(crashDumpFileName, std::ios::out | std::ios::binary);
			if (dumpFile)
			{
				dumpFile.write((const char*)pGpuCrashDump, gpuCrashDumpSize);
				dumpFile.close();
			}

			// Decode the crash dump to a JSON string.
			// Step 1: Generate the JSON and get the size.
			uint32_t jsonSize = 0;
			AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_GenerateJSON(
				decoder,
				GFSDK_Aftermath_GpuCrashDumpDecoderFlags_ALL_INFO,
				GFSDK_Aftermath_GpuCrashDumpFormatterFlags_NONE,
				ShaderDebugInfoLookupCallback,
				ShaderLookupCallback,
				ShaderSourceDebugInfoLookupCallback,
				this,
				&jsonSize));
			// Step 2: Allocate a buffer and fetch the generated JSON.
			std::vector<char> json(jsonSize);
			AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_GetJSON(
				decoder,
				uint32_t(json.size()),
				json.data()));

			// Write the crash dump data as JSON to a file.
			const std::string jsonFileName = crashDumpFileName + ".json";
			std::ofstream jsonFile(jsonFileName, std::ios::out | std::ios::binary);
			if (jsonFile)
			{
				// Write the JSON to the file (excluding string termination)
				jsonFile.write(json.data(), json.size() - 1);
				jsonFile.close();
			}

			// Destroy the GPU crash dump decoder object.
			AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GpuCrashDump_DestroyDecoder(decoder));
		}

		void RHI_GPUCrashTrackerNvidiaAftermath::WriteShaderDebugInformationToFile(GFSDK_Aftermath_ShaderDebugInfoIdentifier identifier, const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize)
		{
			// Create a unique file name.
			const std::string filePath = "shader-" + std::to_string(identifier) + ".nvdbg";

			std::ofstream f(c_shaderFolder + filePath, std::ios::out | std::ios::binary);
			if (f)
			{
				f.write((const char*)pShaderDebugInfo, shaderDebugInfoSize);
				f.close();
			}
		}

		void RHI_GPUCrashTrackerNvidiaAftermath::OnShaderDebugInfoLookup(const GFSDK_Aftermath_ShaderDebugInfoIdentifier& identifier, PFN_GFSDK_Aftermath_SetData setShaderDebugInfo) const
		{
			// Search the list of shader debug information blobs received earlier.
			//auto i_debugInfo = m_shaderDebugInfo.find(identifier);
			//if (i_debugInfo == m_shaderDebugInfo.end())
			//{
			//	// Early exit, nothing found. No need to call setShaderDebugInfo.
			//	return;
			//}
			//
			//// Let the GPU crash dump decoder know about the shader debug information
			//// that was found.
			//setShaderDebugInfo(i_debugInfo->second.data(), uint32_t(i_debugInfo->second.size()));
		}

		void RHI_GPUCrashTrackerNvidiaAftermath::OnShaderLookup(const GFSDK_Aftermath_ShaderBinaryHash& shaderHash, PFN_GFSDK_Aftermath_SetData setShaderBinary) const
		{
			// Find shader binary data for the shader hash in the shader database.
			//std::vector<uint8_t> shaderBinary;
			//if (!m_shaderDatabase.FindShaderBinary(shaderHash, shaderBinary))
			//{
			//	// Early exit, nothing found. No need to call setShaderBinary.
			//	return;
			//}
			//
			//// Let the GPU crash dump decoder know about the shader data
			//// that was found.
			//setShaderBinary(shaderBinary.data(), uint32_t(shaderBinary.size()));
		}

		void RHI_GPUCrashTrackerNvidiaAftermath::OnShaderSourceDebugInfoLookup(const GFSDK_Aftermath_ShaderDebugName& shaderDebugName, PFN_GFSDK_Aftermath_SetData setShaderBinary) const
		{
			// Find source debug info for the shader DebugName in the shader database.
			std::vector<uint8_t> shaderBinary;
			//if (!m_shaderDatabase.FindShaderBinaryWithDebugData(shaderDebugName, shaderBinary))
			//{
			//	// Early exit, nothing found. No need to call setShaderBinary.
			//	return;
			//}
			//
			//// Let the GPU crash dump decoder know about the shader debug data that was
			//// found.
			//setShaderBinary(shaderBinary.data(), uint32_t(shaderBinary.size()));
		}

		void RHI_GPUCrashTrackerNvidiaAftermath::GpuCrashDumpCallback(const void* pGpuCrashDump, const uint32_t gpuCrashDumpSize, void* pUserData)
		{
			RHI_GPUCrashTrackerNvidiaAftermath* pGpuCrashTracker = reinterpret_cast<RHI_GPUCrashTrackerNvidiaAftermath*>(pUserData);
			pGpuCrashTracker->OnCrashDump(pGpuCrashDump, gpuCrashDumpSize);
		}

		void RHI_GPUCrashTrackerNvidiaAftermath::ShaderDebugInfoCallback(const void* pShaderDebugInfo, const uint32_t shaderDebugInfoSize, void* pUserData)
		{
			RHI_GPUCrashTrackerNvidiaAftermath* pGpuCrashTracker = reinterpret_cast<RHI_GPUCrashTrackerNvidiaAftermath*>(pUserData);
			pGpuCrashTracker->OnShaderDebugInfo(pShaderDebugInfo, shaderDebugInfoSize);
		}

		void RHI_GPUCrashTrackerNvidiaAftermath::CrashDumpDescriptionCallback(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription, void* pUserData)
		{
			RHI_GPUCrashTrackerNvidiaAftermath* pGpuCrashTracker = reinterpret_cast<RHI_GPUCrashTrackerNvidiaAftermath*>(pUserData);
			pGpuCrashTracker->OnDescription(addDescription);
		}

		void RHI_GPUCrashTrackerNvidiaAftermath::ResolveMarkerCallback(const void* pMarker, void* pUserData, void** resolvedMarkerData, uint32_t* markerSize)
		{
			RHI_GPUCrashTrackerNvidiaAftermath* pGpuCrashTracker = reinterpret_cast<RHI_GPUCrashTrackerNvidiaAftermath*>(pUserData);
			pGpuCrashTracker->OnResolveMarker(pMarker, resolvedMarkerData, markerSize);
		}

		void RHI_GPUCrashTrackerNvidiaAftermath::ShaderDebugInfoLookupCallback(const GFSDK_Aftermath_ShaderDebugInfoIdentifier* pIdentifier, PFN_GFSDK_Aftermath_SetData setShaderDebugInfo, void* pUserData)
		{
			RHI_GPUCrashTrackerNvidiaAftermath* pGpuCrashTracker = reinterpret_cast<RHI_GPUCrashTrackerNvidiaAftermath*>(pUserData);
			pGpuCrashTracker->OnShaderDebugInfoLookup(*pIdentifier, setShaderDebugInfo);
		}

		void RHI_GPUCrashTrackerNvidiaAftermath::ShaderLookupCallback(const GFSDK_Aftermath_ShaderBinaryHash* pShaderHash, PFN_GFSDK_Aftermath_SetData setShaderBinary, void* pUserData)
		{
			RHI_GPUCrashTrackerNvidiaAftermath* pGpuCrashTracker = reinterpret_cast<RHI_GPUCrashTrackerNvidiaAftermath*>(pUserData);
			pGpuCrashTracker->OnShaderLookup(*pShaderHash, setShaderBinary);
		}

		void RHI_GPUCrashTrackerNvidiaAftermath::ShaderSourceDebugInfoLookupCallback(const GFSDK_Aftermath_ShaderDebugName* pShaderDebugName, PFN_GFSDK_Aftermath_SetData setShaderBinary, void* pUserData)
		{
			RHI_GPUCrashTrackerNvidiaAftermath* pGpuCrashTracker = reinterpret_cast<RHI_GPUCrashTrackerNvidiaAftermath*>(pUserData);
			pGpuCrashTracker->OnShaderSourceDebugInfoLookup(*pShaderDebugName, setShaderBinary);
		}
#endif
	}
}