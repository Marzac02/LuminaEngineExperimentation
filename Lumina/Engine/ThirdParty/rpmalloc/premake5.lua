project "RPMalloc"
	kind "StaticLib"
	warnings "off"
    

	files
	{
		"**.h",
		"**.c",
	}

	includedirs
	{
		".",
	}