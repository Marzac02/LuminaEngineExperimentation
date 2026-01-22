project "JoltPhysics"
	kind "StaticLib"
	warnings "off"
    targetdir ("%{wks.location}/Binaries/" .. outputdir)
    objdir ("%{wks.location}/Intermediates/Obj/" .. outputdir .. "/%{prj.name}")    
    location(ProjectFilesDir)

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
	}

	includedirs
	{
		"."
	}