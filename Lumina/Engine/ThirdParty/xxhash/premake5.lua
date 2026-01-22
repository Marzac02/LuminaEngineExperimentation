project "XXHash"
	kind "StaticLib"
	warnings "off"
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