local LuminaDir = os.getenv("LUMINA_DIR")
print(LuminaDir)

include (path.join(LuminaDir, "BuildScripts/Dependencies"))
include (path.join(LuminaDir, "BuildScripts/Actions/Reflection"))

workspace "MyProject"
	language "C++"
	cppdialect "C++latest"
    warnings "Default"
    targetdir (LuminaConfig.GetTargetDirectory())
    objdir (LuminaConfig.GetObjDirectory())
    enableunitybuild "Off"
    fastuptodate "On"
    multiprocessorcompile "On"
	startproject "MyProject"

	configurations 
    { 
        "Debug",
        "Development",
        "Shipping",
    }

	platforms
    {
        "Editor",
        "Game",
    }


project "MyProject"
	kind "SharedLib"
	rtti "off"
	enablereflection "On"

	libdirs
	{
		LuminaConfig.EnginePath("Engine/Source/ThirdParty/lua"),
	}

	filter "platforms:Editor"
		links "Editor"
		includedirs
		{
			LuminaConfig.EnginePath("Engine/Editor/Source")
		}
	filter {}

	links
	{
		"Runtime",
		"ImGui",
		"RPMalloc",
    	"EA",
    	"EnkiTS",
		"Tracy",
		"lua54",
	}
	 
	files
	{
		"Source/**.h",
		"Source/**.cpp",
		LuminaConfig.GetReflectionFiles(),
		"**.lua",
		"**.lproject",
		"**.json",
	}

	forceincludes
	{
		"MyProjectAPI.h"
	}

	includedirs
	{
		"Source",
	    LuminaConfig.GetPublicIncludeDirectories(),
	}
	 

