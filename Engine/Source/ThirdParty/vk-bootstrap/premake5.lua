project "VKBootstrap"
	kind "StaticLib"
	warnings "off"


	files
	{
		"**.cpp",
		"**.h",
		"**.lua",
	}

	includedirs
	{
		LuminaConfig.ThirdPartyDirectory(),
	}