project "MeshOptimizer"
	kind "StaticLib"
	warnings "off"
    targetdir ("%{wks.location}/Binaries/" .. outputdir)
    objdir ("%{wks.location}/Intermediates/Obj/" .. outputdir .. "/%{prj.name}")    
    location(ProjectFilesDir)

	files
	{
		"src/**.cpp",
		"src/**.h",
	}

	includedirs
	{
		"src/",
	}