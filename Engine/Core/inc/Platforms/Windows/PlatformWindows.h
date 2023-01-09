#pragma once

#ifdef IS_PLATFORM_WINDOWS

#include "Core/TypeAlias.h"
#include "Core/Defines.h"
#include "Core/GUID.h"

#include "Core/CPUInformation.h"

#include <string>
#include <vector>

namespace Insight
{
	namespace Windows
	{
		class IS_CORE PlatformWindows
		{
		public:

			static void Initialise();
			static void Shutdown();

			static void MemCopy(void* dst, void const* src, u64 size);
			static void MemSet(void* dst, int value, u64 size);
			static void MemClear(void* dst, u64 size);
			static bool MemCompare(const void* buf1, const void* buf2, u64 size);

			static bool StrCompare(const char* str1, const char* str2);
			static bool WStrCompare(const wchar_t* wstr1, const wchar_t* wstr2);

			static std::vector<std::string> GetCallStack(int frameCount = 32);

			static std::string StringFromWString(const std::wstring& wstring);
			static std::wstring WStringFromString(const std::string& string);
			static std::wstring WStringFromStringView(std::string_view string);
			
			static Core::GUID CreateGUID();
			static void AssignGUID(Core::GUID& guid);

			static bool IsMainThread();

			static Core::CPUInformation GetCPUInformation();
			static Core::MemoryInformation GetMemoryInformation();

		private:
			static u32 s_mainThreadId;
			static Core::CPUInformation s_cpuInformation;
		};

	}
}
#endif /// IS_PLATFORM_WINDOWS