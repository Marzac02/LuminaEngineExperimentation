include(os.getenv("LUMINA_DIR") .. "/Dependencies.lua")


project "EA"
	kind "StaticLib"
	language "C++"
	enableunitybuild "On"

    targetdir ("%{wks.location}/Binaries/" .. outputdir)
    objdir ("%{wks.location}/Intermediates/Obj/" .. outputdir .. "/%{prj.name}")

    defines
    {
        "LUMINA_ENGINE",
    }

	files
	{
		"**.h",
		"**.cpp"
	}
	
	includedirs
	{
		"EABase",
		"EASTL",
		"EABase/include/Common",
		"EASTL/include/",
	}
