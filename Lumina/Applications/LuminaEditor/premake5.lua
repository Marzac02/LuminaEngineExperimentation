project "Editor"
	kind "ConsoleApp"
    rtti "off"
	dependson { "Reflector", "Lumina", "ImGui", "EA", "Tracy", "lua54" }
	
    libdirs
    {
        "%{LuminaConfig.EngineDirectory}/Lumina/Engine/ThirdParty/lua/",
		"%{LuminaConfig.EngineDirectory}/Lumina/Engine/ThirdParty/NvidiaAftermath/lib",
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
		
		LuminaConfig.GetReflectionUnityFile()
	}

	includedirs
	{
		LuminaConfig.GetPublicIncludeDirectories(),
	}