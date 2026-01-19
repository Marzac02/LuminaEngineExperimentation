project "RPMalloc"
	kind "StaticLib"
	warnings "off"
	enableunitybuild "On"
    targetdir ("%{wks.location}/Binaries/" .. outputdir)
    objdir ("%{wks.location}/Intermediates/Obj/" .. outputdir .. "/%{prj.name}")    

	files
	{
		"**.h",
		"**.c",
	}

	includedirs
	{
		".",
	}