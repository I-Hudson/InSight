local local_post_build_commands = post_build_commands

project "Insight_Graphics"  
    language "C++"
    cppdialect "C++17"
    configurations { "Debug", "Release" } 
    location "./"

    targetname ("%{prj.name}" .. output_project_subfix)
    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")
    debugdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")

    dependson 
    {
        "Insight_Core",
    }

    defines
    {
        "IS_EXPORT_GRAPHICS_DLL",
        "COMMAND_LIST_RENDER_BATCH"
    }
    
    includedirs
    {
        "inc",
        "%{IncludeDirs.InsightCore}",

        "%{IncludeDirs.glfw}",
        "%{IncludeDirs.glm}",
        "%{IncludeDirs.imgui}",
        "%{IncludeDirs.spdlog}",
        "%{IncludeDirs.vma}",
        "%{IncludeDirs.glslang}win_debug/inc",
        "%{IncludeDirs.vulkan}",
        "%{IncludeDirs.spirv_reflect}",
        "%{IncludeDirs.dxcompiler}",
        "%{IncludeDirs.assimp}",
        "%{IncludeDirs.assimp}/../build/include",
        "%{IncludeDirs.optick}",
        "%{IncludeDirs.stb_image}",
        "%{IncludeDirs.meshoptimizer}",
    }

    files 
    { 
        "inc/**.hpp", 
        "inc/**.h", 
        "src/**.cpp",

        "../../vendor/SPIRV-Reflect/spirv_reflect.h",
        "../../vendor/SPIRV-Reflect/spirv_reflect.cpp",
        "../../vendor/stb/stb_image.h",
        "../../vendor/stb/stb_image_write.h",
    }

    links
    {
        "Insight_Core.lib",
        
        "GLFW.lib",
        "glm.lib",
        "imgui.lib",
        "vulkan-1.lib",
        "D3d12.lib",
        "DXGI.lib",
        "dxguid.lib",
        "dxcompiler.lib",
        "Optickcore.lib",
        "tracy.lib",
        "meshoptimizer.lib",
    }

    libdirs
    {
        "%{LibDirs.vulkan}",
    }

    postbuildcommands "%{concat_table(local_post_build_commands)}"

    filter "configurations:Debug or configurations:Testing"
        defines { "DEBUG" }
        symbols "On" 
        links
        {
            "GenericCodeGend.lib",
            "glslangd.lib",
            "glslang-default-resource-limitsd.lib",
            "HLSLd.lib",
            "MachineIndependentd.lib",
            "OGLCompilerd.lib",
            "OSDependentd.lib",
            "SPIRVd.lib",
            "SPIRV-Toolsd.lib",
            "SPIRV-Tools-optd.lib",
            "SPVRemapperd.lib",
            "assimpd.lib",
        }

    filter "configurations:Release"  
        defines { "NDEBUG" }    
        optimize "On"   
        links
        {
            "glslang.lib",
            "MachineIndependent.lib",
            "GenericCodeGen.lib",
            "OGLCompiler.lib",
            "OSDependent.lib",
            "Optickcore.lib",
            "tracy.lib",
            "assimp.lib",
        }

    filter "configurations:Testing" 
        links
        {
            "doctest.lib",
        }
        libdirs
        {
            "%{LibDirs.deps_testing_lib}",
        }