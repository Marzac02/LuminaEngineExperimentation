project "Lumina"
	kind "WindowedApp"
	rtti "off"
	dependson { "Lumina", "Editor", "ImGui", "EA", "Tracy", "lua54" }

	defines
	{ 
		"_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS",
		"TRACY_ENABLE",
    	"TRACY_CALLSTACK",
    	"TRACY_ON_DEMAND",
		"TRACY_IMPORTS",
	}

	libdirs
	{
		LuminaConfig.EnginePath("Engine/Source/ThirdParty/lua"),
	}

	links
	 {
		"Runtime",
		"Editor",
		"ImGui",
    	"EA",
    	"EnkiTS",
		"Tracy",
		"lua54",
	}
	 
	files
	{
		"Source/**.h",
		"Source/**.cpp",
		"**.lua",
	}

	forceincludes
	{
		"LaunchAPI.h"
	}

	includedirs
	{
        LuminaConfig.EnginePath("Engine/Editor/Source"),
	    LuminaConfig.GetPublicIncludeDirectories()
	}
	 