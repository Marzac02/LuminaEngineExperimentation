project "RPMalloc"
	kind "StaticLib"
	warnings "off"
	enableunitybuild "On"
    targetdir ("%{wks.location}/Binaries/" .. outputdir)
    objdir ("%{wks.location}/Intermediates/Obj/" .. outputdir .. "/%{prj.name}")    
    location(ProjectFilesDir)

	files
	{
		"**.h",
		"**.c",
	}

	includedirs
	{
		".",
	}