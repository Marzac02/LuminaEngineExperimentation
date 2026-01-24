project "BuildScripts"
	kind "Utility"    
	
	files
	{
		"**.lua",
        "**.py",
        "../premake5.lua",
	}

	vpaths
	{
		["Lua"] 	= {"**.lua", "../premake5.lua" },
		["Python"] 	= "**.py",
	}