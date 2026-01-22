project "Tracy"
	kind "SharedLib"
	language "C++"
    targetdir ("%{wks.location}/Binaries/" .. outputdir)
    objdir ("%{wks.location}/Intermediates/Obj/" .. outputdir .. "/%{prj.name}")
    location(ProjectFilesDir)

	defines
	{
		"TRACY_EXPORTS",
		"TRACY_ALLOW_SHADOW_WARNING",
	}

	files
	{
		"public/TracyClient.cpp",
	}
	
	includedirs
	{
		"public",
	}