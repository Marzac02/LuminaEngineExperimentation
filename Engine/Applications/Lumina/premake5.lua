project "Lumina"
	kind "WindowedApp"
	rtti "off"

	defines
	{ 
		"TRACY_ENABLE",
    	"TRACY_CALLSTACK",
    	"TRACY_ON_DEMAND",
		"TRACY_IMPORTS",
	}

	links
	{
		"Runtime",
		"Editor",
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
	 