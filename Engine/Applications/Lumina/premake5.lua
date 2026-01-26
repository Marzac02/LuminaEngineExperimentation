project "Lumina"
	kind "WindowedApp"
	rtti "off"

	filter "platforms:Editor"
		links "Editor"
		includedirs { LuminaConfig.EnginePath("Engine/Editor/Source") }
	filter {}

	links
	{
		"Runtime",
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
		"Source",
	    LuminaConfig.GetPublicIncludeDirectories()
	}
	 