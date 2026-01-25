project "FastGLTF"
	kind "StaticLib"
	warnings "off"
    


	files
	{
		"**.hpp",
		"**.h",
		"**.cpp",
		"**.lua",
	}

	includedirs
	{
		"include",
		".",
	}