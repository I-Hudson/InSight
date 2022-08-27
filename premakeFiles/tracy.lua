project "tracy"
	kind "SharedLib"
	language "C++"
    cppdialect "C++14"
    configurations { "Debug", "Release" } 

    targetdir (outputdir_target .. "/%{prj.name}")
    objdir (outputdir_obj.. "/%{prj.name}")
    debugdir (outputdir_debug .. "/%{prj.name}")

	folderDirTracy = "../vendor/tracy/"
	location "%{folderDirTracy}"

	files 
	{
        folderDirTracy.. "public/TracyClient.cpp", 
	}

	includedirs
	{
		folderDirTracy
	}

	defines 
	{ 
		"_CRT_SECURE_NO_WARNINGS",
		"TRACY_ENABLE",
		"TRACY_EXPORTS",
		"TRACY_ON_DEMAND",
	}
	
	postbuildcommands
    {
       "{COPY} \"%{cfg.targetdir}/tracy.dll\" \"" .. output_deps .. "/dll/\"",
       "{COPY} \"%{cfg.targetdir}/tracy.lib\" \"" .. output_deps .. "/lib/\"",
    }

	filter "configurations:Debug"
		defines
		{ 
		}

	filter "configurations:Release"
		optimize "Speed"
		defines
		{ 
		}