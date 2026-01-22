project "EA"
	kind "StaticLib"
	language "C++"
	enableunitybuild "On"
    

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
