project "BuildScripts"
	kind "None"    
	
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