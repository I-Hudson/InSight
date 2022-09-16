local profileTool="tracy"
local monolith_build="false"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
outputdir_target = "/bin/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
outputdir_obj = "/bin-int/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
outputdir_debug = "bin/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
output_executable = "InsightEditor"
output_project_subfix = ""

post_build_commands = {}
function concat_table(table_to_concat)
    return table.concat(table_to_concat, " ")
end

function tprint (tbl, indent)
    if not indent then indent = 0 end
    local toprint = string.rep(" ", indent) .. "{\r\n"
    indent = indent + 2 
    for k, v in pairs(tbl) do
      toprint = toprint .. string.rep(" ", indent)
      if (type(k) == "number") then
        toprint = toprint .. "[" .. k .. "] = "
      elseif (type(k) == "string") then
        toprint = toprint  .. k ..  "= "   
      end
      if (type(v) == "number") then
        toprint = toprint .. v .. ",\r\n"
      elseif (type(v) == "string") then
        toprint = toprint .. "\"" .. v .. "\",\r\n"
      elseif (type(v) == "table") then
        toprint = toprint .. tprint(v, indent + 2) .. ",\r\n"
      else
        toprint = toprint .. "\"" .. tostring(v) .. "\",\r\n"
      end
    end
    toprint = toprint .. string.rep(" ", indent-2) .. "}"
    return toprint
  end

VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDirs = {}
IncludeDirs["InsightCore"] = "%{wks.location}/InsightCore/inc"
IncludeDirs["InsightGraphics"] = "%{wks.location}/InsightGraphics/inc"
IncludeDirs["InsightInput"] = "%{wks.location}/InsightInput/inc"
IncludeDirs["InsightECS"] = "%{wks.location}/InsightECS/inc"
IncludeDirs["InsightApp"] = "%{wks.location}/InsightApp/inc"
IncludeDirs["InsightEditor"] = "%{wks.location}/InsightEditor/inc"

IncludeDirs["doctest"] = "%{wks.location}/vendor/doctest/doctest"
IncludeDirs["glfw"] = "%{wks.location}/Vendor/glfw/include"
IncludeDirs["glm"] = "%{wks.location}/Vendor/glm"
IncludeDirs["imgui"] = "%{wks.location}/Vendor/imgui"
IncludeDirs["spdlog"] = "%{wks.location}/Vendor/spdlog/include"
IncludeDirs["vma"] = "%{wks.location}/vendor/VulkanMemoryAllocator/src"
IncludeDirs["vulkan"] = VULKAN_SDK .. "/include/"
IncludeDirs["spirv_reflect"] = "%{wks.location}/vendor/SPIRV-Reflect"
IncludeDirs["dxcompiler"] = "%{wks.location}/vendor/dxcompiler/win_debug/inc"
IncludeDirs["assimp"] = "%{wks.location}/vendor/assimp/include"
IncludeDirs["optick"] = "%{wks.location}/vendor/optick/src"
IncludeDirs["tracy"] = "%{wks.location}/vendor/tracy"
IncludeDirs["stb_image"] = "%{wks.location}/vendor/stb"
IncludeDirs["meshoptimizer"] = "%{wks.location}/vendor/meshoptimizer/src"

LibDirs = {}

LibDirs["deps_lib"] = "%{wks.location}/deps/" .. outputdir .. "/lib/"
LibDirs["deps_testing_lib"] = "%{wks.location}/deps/Debug-windows-x86_64/lib/"

LibDirs["glslang_win_d"] = "%{wks.location}/vendor/glslang/win_debug/lib"
LibDirs["glslang_win"] = "%{wks.location}/vendor/glslang/win_release/lib"
LibDirs["imgui"] = "%{wks.location}/vendor/imgui/" .. outputdir .. "ImGui/"
LibDirs["dxcompiler_win_d"] = "%{wks.location}/vendor/dxcompiler/win_debug/lib/x64"
LibDirs["dxcompiler_win"] = "%{wks.location}/vendor/dxcompiler/win_release/lib/x64"
LibDirs["vulkan"] = VULKAN_SDK .. "/lib/"

workspace "Insight"
    architecture "x64"
    startproject "InsightEditor"
    staticruntime "off"

    configurations
    {
        "Debug",
        "Release",
        "Testing",
    }
    platforms 
    { 
        "Win64", 
        "Linux" 
    }

    flags
    {
    	"MultiProcessorCompile"
    }

    defines
    {
        "_CRT_SECURE_NO_WARNINGS",
        "IS_PLATFORM_X64",
        "RENDER_GRAPH_ENABLED",
        "GLM_FORCE_SWIZZLE",
        "GLM_FORCE_LEFT_HANDED",
        "GLM_FORCE_DEPTH_ZERO_TO_ONE",
    }

    includedirs
    {
        "%{IncludeDirs.optick}",
        "%{IncludeDirs.tracy}",
        "%{IncludeDirs.doctest}",
    }

    if (profileTool == "tracy") then
        defines { "IS_PROFILE_ENABLED", "IS_PROFILE_TRACY", "TRACY_IMPORTS", "TRACY_ON_DEMAND", }
        editandcontinue "off"
    end
    if (profileTool == "optick") then
        defines { "IS_PROFILE_ENABLED", "IS_PROFILE_OPTICK" }
    end

    libdirs
    {
        "%{LibDirs.deps_lib}",
    }

    if (monolith_build == "false") then
        kind "SharedLib"
        table.insert(post_build_commands, "{COPYFILE} \"%{cfg.targetdir}/%{prj.name}" .. output_project_subfix .. ".dll\" \"%{wks.location}deps/".. outputdir..  "/dll/\"\n")
        table.insert(post_build_commands, "{COPYFILE} \"%{cfg.targetdir}/%{prj.name}" .. output_project_subfix .. ".lib\" \"%{wks.location}deps/".. outputdir..  "/lib/\"\n")
        table.insert(post_build_commands, "{COPYFILE} \"%{cfg.targetdir}/%{prj.name}" .. output_project_subfix .. ".dll\" \"%{wks.location}bin/".. outputdir..  "/" .. output_executable .. "\"\n")
    end
    if (monolith_build == "true") then
        defines { "IS_MONOLITH" }
        kind "StaticLib"
        output_project_subfix = "_monolith"
        table.insert(post_build_commands, "{COPYFILE} \"%{cfg.targetdir}/%{prj.name}" .. output_project_subfix .. ".lib\" \"%{wks.location}deps/".. outputdir..  "/lib/\"")
    end

    filter "configurations:Debug"
        defines
        {
            "DOCTEST_CONFIG_DISABLE",
        }

    filter { "configurations:Debug", "configurations:Testing" }
        buildoptions "/MDd"
        defines
        {
            "_DEBUG"
        }

    filter "configurations:Release"
        buildoptions "/MD"
        defines
        {
            "NDEBUG",
            "DOCTEST_CONFIG_DISABLE",
        }

    filter "system:Windows"
    	system "windows"
    	toolset("msc-v142")
        defines
        {
            "IS_PLATFORM_WINDOWS",
            "IS_PLATFORM_WIN32",
            "VK_USE_PLATFORM_WIN32_KHR",
            --"IS_DX12_ENABLED",
            "IS_VULKAN_ENABLED",
            "NOMINMAX",
        }


    filter "configurations:Testing"
        defines
        {
            "TESTING",
            "DOCTEST_CONFIG_IMPLEMENTATION_IN_DLL",
        }
        files 
        { 
            "vendor/doctest/doctest/doctest.h",
        } 
    	
    
    filter "system:Unix"
    	system "linux"
    	toolset("clang")
        defines
        {
            "IS_PLATFORM_LINUX",
            "IS_VULKAN_ENABLED",
        }

group "Runtime"
    include "InsightCore/InsightCore.lua"
    include "InsightGraphics/InsightGraphics.lua"
    include "InsightInput/InsightInput.lua"
    include "InsightECS/InsightECS.lua"
    include "InsightApp/InsightApp.lua"

group "Editor"
    include "InsightEditor/InsightEditor.lua"