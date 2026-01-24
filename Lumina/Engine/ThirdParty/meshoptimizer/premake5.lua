project "MeshOptimizer"
	kind "StaticLib"
	warnings "off"
    

	files
	{
		"src/**.cpp",
		"src/**.h",
		"**.lua",
	}

	includedirs
	{
		"src/",
	}