project "VKBootstrap"
	kind "StaticLib"
	warnings "off"
    

	files
	{
		"**.cpp",
		"**.h",
	}

	includedirs
	{
		".",
		LuminaConfig.ThirdPartyDirectory(),
	}