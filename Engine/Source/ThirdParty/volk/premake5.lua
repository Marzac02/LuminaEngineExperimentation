project "Volk"
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
		LuminaConfig.ThirdPartyDirectory(),
	}