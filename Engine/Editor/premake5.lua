project "Editor"
	kind "SharedLib"
    rtti "off"
	dependson { "Reflector", "ImGui", "EA", "Tracy", "lua54" }
	enablereflection "true"
	removeplatforms { "Game" }

    libdirs
    {
        LuminaConfig.EnginePath("Engine/Source/ThirdParty/lua"),
		LuminaConfig.EnginePath("Engine/Source/ThirdParty/NvidiaAftermath/lib"),
    }

	links
	{
		"Runtime",
		"ImGui",
		"RPMalloc",
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

	forceincludes
	{
		"EditorAPI.h",
	}

	includedirs
	{
        "Source",
		LuminaConfig.GetPublicIncludeDirectories(),
	}