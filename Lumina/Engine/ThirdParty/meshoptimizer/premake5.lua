project "MeshOptimizer"
	kind "StaticLib"
	warnings "off"
    

	files
	{
		"src/**.cpp",
		"src/**.h",
	}

	includedirs
	{
		"src/",
	}