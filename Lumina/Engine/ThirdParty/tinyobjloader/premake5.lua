project "TinyOBJLoader"
	kind "StaticLib"
	warnings "off"
    targetdir ("%{wks.location}/Binaries/" .. outputdir)
    objdir ("%{wks.location}/Intermediates/Obj/" .. outputdir .. "/%{prj.name}")    

	files
	{
		"**.cc",
		"**.h",
	}

	includedirs
	{
		".",
	}