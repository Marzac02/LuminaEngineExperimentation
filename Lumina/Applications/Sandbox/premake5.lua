project "Sandbox"
	kind "ConsoleApp"
	rtti "off"
	dependson { "Reflector", "Lumina", "ImGui", "EA", "Tracy", "lua54" }

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
		"%{LuminaConfig.EngineDirectory}/Lumina/Engine/ThirdParty/lua/",
	}

	links
	 {
		"Lumina",
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
		--LuminaConfig.GetReflectionUnityFile()
		"**.lua",
	}

	includedirs
	{
	    LuminaConfig.GetPublicIncludeDirectories()
	}
	 