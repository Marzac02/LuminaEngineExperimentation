project "Tracy"
	kind "SharedLib"
	language "C++"
    

	defines
	{
		"TRACY_EXPORTS",
		"TRACY_ALLOW_SHADOW_WARNING",
	}

	files
	{
		"public/TracyClient.cpp",
	}
	
	includedirs
	{
		"public",
	}