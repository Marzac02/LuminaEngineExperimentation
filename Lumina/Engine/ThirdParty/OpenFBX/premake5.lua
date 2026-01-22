project "OpenFBX"
	kind "StaticLib"
	warnings "off"
    targetdir ("%{wks.location}/Binaries/" .. outputdir)
    objdir ("%{wks.location}/Intermediates/Obj/" .. outputdir .. "/%{prj.name}")    
    location(ProjectFilesDir)

	files
	{
		"**.h",
		"**.cpp",
		"**.c",
	}

	includedirs
	{
		".",
	}