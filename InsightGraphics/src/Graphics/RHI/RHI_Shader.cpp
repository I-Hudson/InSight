#include "Graphics/RHI/RHI_Shader.h"
#include "Core/Memory.h"
#include "Graphics/GraphicsManager.h"

#include "Graphics/RHI/Vulkan/RHI_Shader_Vulkan.h"
#include "Graphics/RHI/DX12/RHI_Shader_DX12.h"
#include "Graphics/RHI/DX12/DX12Utils.h"

#include "dxc/dxcapi.h"
#include "spirv_reflect.h"

#include <fstream>

namespace Insight
{
	namespace Graphics
	{
		RHI_Shader* RHI_Shader::New()
		{
#if defined(IS_VULKAN_ENABLED)
			if (GraphicsManager::IsVulkan()) { return NewTracked(RHI::Vulkan::RHI_Shader_Vulkan); }
#endif
#if defined(IS_DX12_ENABLED)
			else if (GraphicsManager::IsDX12()) { return NewTracked(RHI::DX12::RHI_Shader_DX12); }
#endif
			return nullptr;
		}


		RHI_ShaderManager::RHI_ShaderManager()
		{
		}

		RHI_ShaderManager::~RHI_ShaderManager()
		{
		}

		RHI_Shader* RHI_ShaderManager::GetOrCreateShader(ShaderDesc desc)
		{
			if (!desc.IsValid())
			{
				return nullptr;
			}
			const u64 hash = desc.GetHash();
			auto itr = m_shaders.find(hash);
			if (itr != m_shaders.end())
			{
				return itr->second;
			}

			RHI_Shader* shader = RHI_Shader::New();
			shader->Create(m_context, desc);
			m_shaders[hash] = shader;

			return shader;
		}

		void RHI_ShaderManager::Destroy()
		{
			for (auto& pair : m_shaders)
			{
				pair.second->Destroy();
				DeleteTracked(pair.second);
			}
			m_shaders.clear();
		}


		/// <summary>
		/// ShaderCompiler
		/// </summary>
		ShaderCompiler::ShaderCompiler()
		{
			HRESULT hres;
			// Initialize DXC library
			hres = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&DXUtils));
			if (FAILED(hres)) 
			{
				throw std::runtime_error("Could not init DXC Library");
			}

			// Initialize the DXC compiler
			hres = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&DXCompiler));
			if (FAILED(hres))
			{
				throw std::runtime_error("Could not init DXC Compiler");
			}
		}

		ShaderCompiler::~ShaderCompiler()
		{
		}

		ComPtr<IDxcBlob> ShaderCompiler::Compile(ShaderStageFlagBits stage, std::wstring_view filePath, ShaderCompilerLanguage languageToCompileTo)
		{
			m_languageToCompileTo = languageToCompileTo;

			// Create default include handler. (You can create your own...)
			ComPtr<IDxcIncludeHandler> pIncludeHandler;
			ASSERT(SUCCEEDED(DXUtils->CreateDefaultIncludeHandler(&pIncludeHandler)));

			// Load the HLSL text shader from disk
			uint32_t codePage = CP_UTF8;
			ComPtr<IDxcBlobEncoding> sourceBlob;
			ASSERT(SUCCEEDED(DXUtils->LoadFile(filePath.data(), nullptr, &sourceBlob)));

			DxcBuffer Source;
			Source.Ptr = sourceBlob->GetBufferPointer();
			Source.Size = sourceBlob->GetBufferSize();
			Source.Encoding = DXC_CP_ACP; // Assume BOM says UTF8 or UTF16 or this is ANSI text.

			// Set up arguments to be passed to the shader compiler

			// Tell the compiler to output SPIR-V
			std::vector<LPCWCHAR> arguments;
			if (languageToCompileTo == ShaderCompilerLanguage::Spirv)
			{
				arguments.push_back(L"-D");
				arguments.push_back(L"VULKAN");
				arguments.push_back(L"-spirv");
			}
			else
			{
				arguments.push_back(L"-D");
				arguments.push_back(L"DX12");
			}

			std::wstring mainFunc = StageToFuncName(stage);
			std::wstring targetProfile = StageToProfileTarget(stage);

			// Entry point
			arguments.push_back(L"-E");
			arguments.push_back(mainFunc.c_str());

			// Select target profile based on shader file extension
			arguments.push_back(L"-T");
			arguments.push_back(targetProfile.c_str());

			const wchar_t* c_Include_Directory = L"-I";
			arguments.push_back(c_Include_Directory);
			arguments.push_back(L"Resources/Shaders/hlsl");

			arguments.push_back(DXC_ARG_DEBUG); //-Zi
			//arguments.push_back(L"-Zs"); // Generate small PDB with just sourcesand compile options.Cannot be used together with - Zi
			arguments.push_back(DXC_ARG_SKIP_OPTIMIZATIONS);

			//arguments.push_back(L"-fvk-bind-register");

			// Compile shader
			ASSERT(SUCCEEDED(DXCompiler->Compile(
				&Source,									// Source buffer.
				arguments.data(),							// Array of pointers to arguments.
				static_cast<UINT>(arguments.size()),		// Number of arguments.
				pIncludeHandler.Get(),						// User-provided interface to handle #include directives (optional).
				IID_PPV_ARGS(&ShaderCompileResults)			// Compiler output status, buffer, and errors.
			)));

			arguments.push_back(L"-spirv");
			ASSERT(SUCCEEDED(DXCompiler->Compile(
				&Source,                // Source buffer.
				arguments.data(),       // Array of pointers to arguments.
				static_cast<UINT>(arguments.size()),		// Number of arguments.
				pIncludeHandler.Get(),	// User-provided interface to handle #include directives (optional).
				IID_PPV_ARGS(&ShaderReflectionResults) // Compiler output status, buffer, and errors.
			)));

			// Print errors if present.
			ComPtr<IDxcBlobUtf8> pErrors = nullptr;
			ASSERT(SUCCEEDED(ShaderCompileResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr)));
			// Note that d3dcompiler would return null if no errors or warnings are present.  
			// IDxcCompiler3::Compile will always return an error buffer, but its length will be zero if there are no warnings or errors.
			if (pErrors != nullptr && pErrors->GetStringLength() != 0)
			{
				IS_CORE_ERROR(fmt::format("Shader compilation failed : \n\n{}", pErrors->GetStringPointer()));
			}

			// Get compilation result
			ComPtr<IDxcBlob> code;
			ShaderCompileResults->GetResult(&code);

			// Write shader to disk.
			std::ofstream shaderDisk;

			int startShaderFile = (int)filePath.find_last_of('/') + 1;
			int offsetShaderFile = (int)filePath.find_last_of('.') - startShaderFile;

			std::wstring_view shaderToDiskView = filePath.substr(startShaderFile, offsetShaderFile);
			std::wstring shaderToDisk = std::wstring(shaderToDiskView) + L".cso";

			shaderDisk.open(shaderToDisk.c_str());
			if (shaderDisk.is_open())
			{
				shaderDisk.write((const char*)code->GetBufferPointer(), code->GetBufferSize());
				shaderDisk.close();
			}

			return code;
		}

		void ShaderCompiler::GetDescriptors(ShaderStageFlagBits stage, std::vector<Descriptor>& descriptors, PushConstant& push_constant)
		{
			ComPtr<IDxcBlob> code;
			ShaderReflectionResults->GetResult(&code);

			// Generate reflection data for a shader
			SpvReflectShaderModule module;
			SpvReflectResult result = spvReflectCreateShaderModule(code->GetBufferSize(), code->GetBufferPointer(), &module);
			assert(result == SPV_REFLECT_RESULT_SUCCESS);

			// Enumerate and extract shader's input variables
			uint32_t var_count = 0;
			result = spvReflectEnumerateInputVariables(&module, &var_count, NULL);
			assert(result == SPV_REFLECT_RESULT_SUCCESS);
			SpvReflectInterfaceVariable** input_vars =
				(SpvReflectInterfaceVariable**)malloc(var_count * sizeof(SpvReflectInterfaceVariable*));
			result = spvReflectEnumerateInputVariables(&module, &var_count, input_vars);
			assert(result == SPV_REFLECT_RESULT_SUCCESS);

			// Output variables, descriptor bindings, descriptor sets, and push constants
			// can be enumerated and extracted using a similar mechanism.

			u32 descriptorSetCount = 0;
			result = spvReflectEnumerateDescriptorSets(&module, &descriptorSetCount, NULL);
			assert(result == SPV_REFLECT_RESULT_SUCCESS);

			std::vector<SpvReflectDescriptorSet*> descriptorSets;
			descriptorSets.resize(descriptorSetCount);
			result = spvReflectEnumerateDescriptorSets(&module, &descriptorSetCount, descriptorSets.data());
			assert(result == SPV_REFLECT_RESULT_SUCCESS);

			auto findPreviousDescriptor = [](const Descriptor& descriptoToFind, const std::vector<Descriptor>& descriptorsToFindIn) 
				-> std::pair<bool, Descriptor*>
			{
				for (auto& desc : descriptorsToFindIn)
				{
					if (descriptoToFind.Set == desc.Set
						&& descriptoToFind.Binding == desc.Binding
						&& descriptoToFind.Size == desc.Size
						&& descriptoToFind.Type == desc.Type
						&& descriptoToFind.ResourceType == desc.ResourceType)
					{
						return std::make_pair(true, &const_cast<Descriptor&>(desc));
					}
				}
				return std::make_pair(false, nullptr);
			};

			for (size_t i = 0; i < descriptorSets.size(); ++i)
			{
				const SpvReflectDescriptorSet& descriptorSet = *descriptorSets[i];
				for (size_t j = 0; j < descriptorSet.binding_count; ++j)
				{
					const SpvReflectDescriptorBinding& binding = *descriptorSet.bindings[j];
					const SpvReflectBlockVariable& block = descriptorSet.bindings[j]->block;

					Descriptor descriptor(
						binding.set,
						binding.binding,
						stage,
						block.size,
						SpvReflectDescriptorTypeToDescriptorType(binding.descriptor_type),
						SpvReflectDescriptorResourceTypeToDescriptorResourceType(binding.resource_type)
					);

					auto [foundDescriptor, descriptorFound] = findPreviousDescriptor(descriptor, descriptors);
					if (foundDescriptor)
					{
						descriptorFound->Stage |= stage;
					}
					else
					{
						descriptors.push_back(descriptor);
					}
				}
			}

			u32 push_constant_blocks_count = 0;
			result = spvReflectEnumeratePushConstantBlocks(&module, &push_constant_blocks_count, NULL);
			assert(result == SPV_REFLECT_RESULT_SUCCESS);
			ASSERT(push_constant_blocks_count <= 1);

			std::vector<SpvReflectBlockVariable*> push_constants;
			push_constants.resize(push_constant_blocks_count);
			result = spvReflectEnumeratePushConstantBlocks(&module, &push_constant_blocks_count, push_constants.data());
			assert(result == SPV_REFLECT_RESULT_SUCCESS);

			for (size_t i = 0; i < push_constants.size(); ++i)
			{
				SpvReflectBlockVariable* block = push_constants.at(i);
				push_constant.Size = block->size;
				push_constant.ShaderStages |= stage;
			}

			// Destroy the reflection data when no longer required.
			spvReflectDestroyShaderModule(&module);

			// Sort all descriptors from 0 to n, by Set then Binding.
			std::sort(descriptors.begin(), descriptors.end(), [](const Descriptor& d1, const Descriptor& d2)
				{
					return d1.Set != d2.Set ? d1.Set < d2.Set
						: d1.Binding < d2.Binding;
				});
		}

		u32 SpvFormatToByteSize(SpvReflectFormat format)
		{
			switch (format)
			{
			case SPV_REFLECT_FORMAT_UNDEFINED: assert(false);
			case SPV_REFLECT_FORMAT_R32_UINT:				return 4;
			case SPV_REFLECT_FORMAT_R32_SINT:				return 4;
			case SPV_REFLECT_FORMAT_R32_SFLOAT:				return 4;
			case SPV_REFLECT_FORMAT_R32G32_UINT:			return 8;
			case SPV_REFLECT_FORMAT_R32G32_SINT:			return 8;
			case SPV_REFLECT_FORMAT_R32G32_SFLOAT:			return 8;
			case SPV_REFLECT_FORMAT_R32G32B32_UINT:			return 12;
			case SPV_REFLECT_FORMAT_R32G32B32_SINT:			return 12;
			case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:		return 12;
			case SPV_REFLECT_FORMAT_R32G32B32A32_UINT:		return 16;
			case SPV_REFLECT_FORMAT_R32G32B32A32_SINT:		return 16;
			case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:	return 16;
			case SPV_REFLECT_FORMAT_R64_UINT:				return 8;
			case SPV_REFLECT_FORMAT_R64_SINT:				return 8;
			case SPV_REFLECT_FORMAT_R64_SFLOAT:				return 8;
			case SPV_REFLECT_FORMAT_R64G64_UINT:			return 16;
			case SPV_REFLECT_FORMAT_R64G64_SINT:			return 16;
			case SPV_REFLECT_FORMAT_R64G64_SFLOAT:			return 16;
			case SPV_REFLECT_FORMAT_R64G64B64_UINT:			return 24;
			case SPV_REFLECT_FORMAT_R64G64B64_SINT:			return 24;
			case SPV_REFLECT_FORMAT_R64G64B64_SFLOAT:		return 24;
			case SPV_REFLECT_FORMAT_R64G64B64A64_UINT:		return 32;
			case SPV_REFLECT_FORMAT_R64G64B64A64_SINT:		return 32;
			case SPV_REFLECT_FORMAT_R64G64B64A64_SFLOAT:	return 32;
			default:  assert(false);
			}
			return 0;
		}

		PixelFormat SpvFormatToPixelFormat(SpvReflectFormat frt)
		{
			switch (frt)
			{
			case SPV_REFLECT_FORMAT_UNDEFINED:				return PixelFormat::Unknown;
			case SPV_REFLECT_FORMAT_R32_UINT:				return PixelFormat::R32_UInt;
			case SPV_REFLECT_FORMAT_R32_SINT:				return PixelFormat::R32_SInt;
			case SPV_REFLECT_FORMAT_R32_SFLOAT:				return PixelFormat::R32_Float;
			case SPV_REFLECT_FORMAT_R32G32_UINT:			return PixelFormat::R32G32_UInt;
			case SPV_REFLECT_FORMAT_R32G32_SINT:			return PixelFormat::R32G32_SInt;
			case SPV_REFLECT_FORMAT_R32G32_SFLOAT:			return PixelFormat::R32G32_Float;
			case SPV_REFLECT_FORMAT_R32G32B32_UINT:			return PixelFormat::R32G32B32_UInt;
			case SPV_REFLECT_FORMAT_R32G32B32_SINT:			return PixelFormat::R32G32B32_SInt;
			case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:		return PixelFormat::R32G32B32_Float;
			case SPV_REFLECT_FORMAT_R32G32B32A32_UINT:		return PixelFormat::R32G32B32A32_UInt;
			case SPV_REFLECT_FORMAT_R32G32B32A32_SINT:		return PixelFormat::R32G32B32A32_SInt;
			case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT:	return PixelFormat::R32G32B32A32_Float;
			case SPV_REFLECT_FORMAT_R64_UINT:				assert(false && "Format not supported.");
			case SPV_REFLECT_FORMAT_R64_SINT:				assert(false && "Format not supported.");
			case SPV_REFLECT_FORMAT_R64_SFLOAT:				assert(false && "Format not supported.");
			case SPV_REFLECT_FORMAT_R64G64_UINT:			assert(false && "Format not supported.");
			case SPV_REFLECT_FORMAT_R64G64_SINT:			assert(false && "Format not supported.");
			case SPV_REFLECT_FORMAT_R64G64_SFLOAT:			assert(false && "Format not supported.");
			case SPV_REFLECT_FORMAT_R64G64B64_UINT:			assert(false && "Format not supported.");
			case SPV_REFLECT_FORMAT_R64G64B64_SINT:			assert(false && "Format not supported.");
			case SPV_REFLECT_FORMAT_R64G64B64_SFLOAT:		assert(false && "Format not supported.");
			case SPV_REFLECT_FORMAT_R64G64B64A64_UINT:		assert(false && "Format not supported.");
			case SPV_REFLECT_FORMAT_R64G64B64A64_SINT:		assert(false && "Format not supported.");
			case SPV_REFLECT_FORMAT_R64G64B64A64_SFLOAT:	assert(false && "Format not supported.");
			}
			return PixelFormat::Unknown;
		}

		std::vector<ShaderInputLayout> ShaderCompiler::GetInputLayout()
		{
			ComPtr<IDxcBlob> code;
			ShaderReflectionResults->GetResult(&code);

			// Generate reflection data for a shader
			SpvReflectShaderModule module;
			SpvReflectResult result = spvReflectCreateShaderModule(code->GetBufferSize(), code->GetBufferPointer(), &module);
			assert(result == SPV_REFLECT_RESULT_SUCCESS);

			// Enumerate and extract shader's input variables
			uint32_t inputCount = 0;
			result = spvReflectEnumerateInputVariables(&module, &inputCount, NULL);
			assert(result == SPV_REFLECT_RESULT_SUCCESS);
			std::vector<SpvReflectInterfaceVariable*> inputVars;
			inputVars.resize(inputCount);

			//SpvReflectInterfaceVariable** input_vars = inputVars (SpvReflectInterfaceVariable**)malloc(inputCount * sizeof(SpvReflectInterfaceVariable*));
			result = spvReflectEnumerateInputVariables(&module, &inputCount, inputVars.data());
			assert(result == SPV_REFLECT_RESULT_SUCCESS);

			std::vector<ShaderInputLayout> inputLayout;
			int stride = 0;

			for (size_t i = 0; i < inputCount; i++)
			{
				SpvReflectInterfaceVariable* interfaceVariable = inputVars.at(i);
				if (interfaceVariable->built_in != -1)
				{
					continue;

				}
				std::string name = interfaceVariable->name;
				name = name.substr(name.find_last_of('.') + 1);

				ShaderInputLayout layout(
					interfaceVariable->location,
					SpvFormatToPixelFormat(interfaceVariable->format),
					stride,
					std::move(name));
				inputLayout.push_back(layout);

				stride += SpvFormatToByteSize(interfaceVariable->format);
			}

			// Destroy the reflection data when no longer required.
			spvReflectDestroyShaderModule(&module);

			return inputLayout;
		}


		std::wstring ShaderCompiler::StageToFuncName(ShaderStageFlagBits stage)
		{
			switch (stage)
			{
			case ShaderStageFlagBits::ShaderStage_Vertex: return L"VSMain";
			case ShaderStageFlagBits::ShaderStage_TessControl: return L"TSMain";
			case ShaderStageFlagBits::ShaderStage_TessEval: return L"TEMain";
			case ShaderStageFlagBits::ShaderStage_Geometry: return L"GSMain";
			case ShaderStageFlagBits::ShaderStage_Pixel: return L"PSMain";
				break;
			}
			return L"";
		}

		std::wstring ShaderCompiler::StageToProfileTarget(ShaderStageFlagBits stage)
		{
			switch (stage)
			{
			case ShaderStageFlagBits::ShaderStage_Vertex: return L"vs_6_1";
			case ShaderStageFlagBits::ShaderStage_TessControl: return L"hs_6_1";
			case ShaderStageFlagBits::ShaderStage_TessEval: return L"te_6_1";
			case ShaderStageFlagBits::ShaderStage_Geometry: return L"gs_6_1";
			case ShaderStageFlagBits::ShaderStage_Pixel: return L"ps_6_1";
				break;
			}
			return L"";
		}

		DescriptorType ShaderCompiler::SpvReflectDescriptorTypeToDescriptorType(u32 type)
		{
			switch (type)
			{
			case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:						return DescriptorType::Sampler;
			case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:		return DescriptorType::Combined_Image_Sampler;
			case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:					return DescriptorType::Sampled_Image;
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:					return DescriptorType::Storage_Buffer;
			case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:			return DescriptorType::Uniform_Texel_Buffer;
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:			return DescriptorType::Storage_Texel_Buffer;
			case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:				return DescriptorType::Unifom_Buffer;
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:				return DescriptorType::Storage_Buffer;
			case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:		return DescriptorType::Uniform_Buffer_Dynamic;
			case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:		return DescriptorType::Storage_Buffer_Dyanmic;
			case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:				return DescriptorType::Input_Attachment;
			case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:	return DescriptorType::Unknown;
			default:
				break;
			}
			return DescriptorType::Unknown;
		}

		DescriptorResourceType ShaderCompiler::SpvReflectDescriptorResourceTypeToDescriptorResourceType(u32 type)
		{
			switch (type)
			{
			case SPV_REFLECT_RESOURCE_FLAG_UNDEFINED:	return DescriptorResourceType::Unknown;
			case SPV_REFLECT_RESOURCE_FLAG_SAMPLER:		return DescriptorResourceType::Sampler;
			case SPV_REFLECT_RESOURCE_FLAG_CBV:			return DescriptorResourceType::CBV;
			case SPV_REFLECT_RESOURCE_FLAG_SRV:			return DescriptorResourceType::SRV;
			case SPV_REFLECT_RESOURCE_FLAG_UAV:			return DescriptorResourceType::UAV;
			default:
				break;
			}
			return DescriptorResourceType::Unknown;
		}
	}
}