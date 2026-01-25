project "Lumina"
	kind "WindowedApp"
	rtti "off"
	dependson { "Reflector", "Lumina", "Editor", "ImGui", "EA", "Tracy", "lua54" }

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
		"Lumina",
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

	includedirs
	{
        LuminaConfig.EnginePath("Engine/Editor/Source"),
	    LuminaConfig.GetPublicIncludeDirectories()
	}
	 