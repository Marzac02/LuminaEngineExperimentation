project "Volk"
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
		LuminaConfig.ThirdPartyDirectory(),
	}