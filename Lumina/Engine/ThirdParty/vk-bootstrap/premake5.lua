project "VKBootstrap"
	kind "StaticLib"
	warnings "off"
    targetdir ("%{wks.location}/Binaries/" .. outputdir)
    objdir ("%{wks.location}/Intermediates/Obj/" .. outputdir .. "/%{prj.name}")    

	files
	{
		"src/**.cpp",
		"src/**.h",
	}

	includedirs
	{
		"src/",
		includedependencies()
	}