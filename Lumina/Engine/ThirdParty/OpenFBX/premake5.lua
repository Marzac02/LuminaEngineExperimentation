project "OpenFBX"
	kind "StaticLib"
	warnings "off"
    

	files
	{
		"**.h",
		"**.cpp",
		"**.c",
		"**.lua",
	}

	includedirs
	{
		".",
	}