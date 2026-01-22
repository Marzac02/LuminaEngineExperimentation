include(os.getenv("LUMINA_DIR") .. "/Dependencies")

project "Sandbox"
	kind "ConsoleApp"
    targetdir ("%{wks.location}/Binaries/" .. outputdir)
    objdir ("%{wks.location}/Intermediates/Obj/" .. outputdir .. "/%{prj.name}")   
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
	    
	    "%{LuminaEngineDirectory}/Lumina/",
		"%{LuminaEngineDirectory}/Lumina/Engine/",
	    "%{LuminaEngineDirectory}/Lumina/Engine/Source/",
	    "%{LuminaEngineDirectory}/Lumina/Engine/Source/Runtime/",

	    reflection_dir,
		includedependencies(),
	}
	 