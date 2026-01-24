project "JoltPhysics"
	kind "StaticLib"
	warnings "off"
    

	defines
	{
		"JPH_DEBUG_RENDERER",
        "JPH_FLOATING_POINT_EXCEPTIONS_ENABLED",
        "JPH_EXTERNAL_PROFILE",
        "JPH_ENABLE_ASSERTS",
	}

	files
	{
		"**.h",
		"**.cpp",
		"**.lua",
	}

	includedirs
	{
		"."
	}