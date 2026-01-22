project "Sandbox"
	kind "ConsoleApp"
	location(ProjectFilesDir)

	defines
	{ 
		"_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS",
		"TRACY_ENABLE",
    	"TRACY_CALLSTACK",
    	"TRACY_ON_DEMAND",
		"TRACY_IMPORTS",
	}

	links
	 {
		"Lumina",
		"ImGui",
    	"EA",
    	"EnkiTS",
		"Tracy",
	 }
	 
	files
	{
		"Source/**.h",
		"Source/**.cpp",
        "%{wks.location}/Intermediates/Reflection/Sandbox/**.cpp",
	}

	includedirs
	{
	    "Source",
	    LuminaConfig.GetPublicIncludeDirectories()
	}
	 