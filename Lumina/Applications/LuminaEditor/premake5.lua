include(os.getenv("LUMINA_DIR") .. "/Dependencies.lua")

project "Editor"
	kind "ConsoleApp"
    rtti "off"
	enableunitybuild "On"
    targetdir ("%{wks.location}/Binaries/" .. outputdir)
    objdir ("%{wks.location}/Intermediates/Obj/" .. outputdir .. "/%{prj.name}")   
	dependson { "Lumina", "ImGui", "EA", "Tracy", "lua54" }

	
    libdirs
    {
        "%{LuminaEngineDirectory}/Lumina/Engine/ThirdParty/lua/",
		"%{LuminaEngineDirectory}/Lumina/Engine/ThirdParty/NvidiaAftermath/lib",
    }

	links
	 {
		"Lumina",
		"ImGui",
    	"EA",
		"Tracy",
		"lua54",
		"GFSDK_Aftermath_Lib",
	 }
	 
	files
	{
		"Source/**.cpp",
		"Source/**.h",
		
		reflection_unity_file,
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