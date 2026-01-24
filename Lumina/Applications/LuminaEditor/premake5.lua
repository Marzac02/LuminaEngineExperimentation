project "Editor"
	kind "ConsoleApp"
    rtti "off"
	dependson { "Reflector", "Lumina", "ImGui", "EA", "Tracy", "lua54" }
	enablereflection "true"
	removeplatforms { "Game" }

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
		"EnkiTS",
		"Tracy",
		"lua54",
		"GFSDK_Aftermath_Lib",
	}

	files
	{
		"Source/**.h",
		"Source/**.cpp",
		"**.lua",
		LuminaConfig.GetReflectionFiles()
	}

	includedirs
	{
		LuminaConfig.GetPublicIncludeDirectories(),
	}