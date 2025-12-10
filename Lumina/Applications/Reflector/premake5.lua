include(os.getenv("LUMINA_DIR") .. "/Dependencies.lua")


project "Reflector"
	kind "ConsoleApp"
    targetdir ("%{LuminaEngineDirectory}/Binaries/" .. outputdir)
    objdir ("%{LuminaEngineDirectory}/Intermediates/Obj/" .. outputdir .. "/%{prj.name}")
	configmap 
	{
		["Debug"] 		= "Development",
		["Shipping"]	= "Development",
	}

	disablewarnings
	{
		"4291" -- memory will not be freed if initialization throws an exception
	}

	prebuildcommands 
	{
		"{MKDIR} \"%{LuminaEngineDirectory}/Binaries/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}\"",
	    "{COPYFILE} \"%{LuminaEngineDirectory}/External/LLVM/bin/libclang.dll\" \"%%{LuminaEngineDirectory}/Binaries/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}/\"",
	}

	libdirs
	{
		"%{LuminaEngineDirectory}/External/LLVM/Lib",
		"%{LuminaEngineDirectory}/External/LLVM/bin",
	}

	links
	{	  	
	  	"clangBasic.lib",
	  	"clangLex.lib",
	  	"clangAST.lib",
	  	"libclang.lib",
	  	"LLVMAnalysis.lib",
	  	"LLVMBinaryFormat.lib",
	  	"LLVMBitReader.lib",
	  	"LLVMBitstreamReader.lib",
	  	"LLVMDemangle.lib",
	  	"LLVMFrontendOffloading.lib",
	  	"LLVMFrontendOpenMP.lib",
	  	"LLVMMC.lib",
	  	"LLVMProfileData.lib",
	  	"LLVMRemarks.lib",
	  	"LLVMScalarOpts.lib",
	  	"LLVMTargetParser.lib",
	  	"LLVMTransformUtils.lib",
	  	"LLVMCore.lib",
        "LLVMSupport.lib",
	}
	  

	files
	{
		"Source/**.cpp",
		"Source/**.h",
		"%{wks.location}/Lumina/Engine/ThirdParty/EA/**.h",
		"%{wks.location}/Lumina/Engine/ThirdParty/EA/**.cpp",
		"%{wks.location}/Lumina/Engine/ThirdParty/xxhash/**.c",
	}


	includedirs
	{ 
		"Source",
	    
	   	"%{LuminaEngineDirectory}/Lumina",
		"%{LuminaEngineDirectory}/Lumina/Engine/ThirdParty/",
		"%{LuminaEngineDirectory}/External/LLVM/include/",
		
		includedependencies(),
	}

