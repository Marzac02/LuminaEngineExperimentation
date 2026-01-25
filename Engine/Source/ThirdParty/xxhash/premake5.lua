project "XXHash"
	kind "StaticLib"
	warnings "off"
    

	files
	{
		"**.h",
		"**.c",
		"**.lua",
	}

	includedirs
	{
		".",
	}