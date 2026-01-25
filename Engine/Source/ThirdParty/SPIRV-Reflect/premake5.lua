project "SPIRV-Reflect"
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