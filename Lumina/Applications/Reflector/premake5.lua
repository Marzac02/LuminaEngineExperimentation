project "Reflector"
	kind "ConsoleApp"
    
	configmap
	{
		["Debug"] 				= "Development",
		["DebugEditor"]			= "Development",
		["DevelopmentEditor"] 	= "Development",
		["Shipping"]			= "Development",
	}

	disablewarnings
	{
		"4291" -- memory will not be freed if initialization throws an exception
	}

	prebuildcommands 
	{
		LuminaConfig.MakeDirectory(LuminaConfig.EnginePath("Binaries/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}")),
		LuminaConfig.CopyFile(LuminaConfig.EnginePath("External/LLVM/bin/libclang.dll"), LuminaConfig.GetTargetDirectory())
	}

	libdirs
	{
		LuminaConfig.EnginePath("External/LLVM/Lib"),
		LuminaConfig.EnginePath("External/LLVM/bin"),
	}

	links
	{
		"NlohmannJson",
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
		LuminaConfig.ThirdPartyPath("EA/**.h"),
		LuminaConfig.ThirdPartyPath("EA/**.cpp"),
		LuminaConfig.ThirdPartyPath("xxhash/**.c"),
		"**.lua",
	}


	includedirs
	{ 
		"Source",
		LuminaConfig.EnginePath("External/LLVM/include/"),
		LuminaConfig.ThirdPartyPath("xxhash"),
		LuminaConfig.ThirdPartyPath("json"),
		LuminaConfig.ThirdPartyPath("spdlog/include"),
		LuminaConfig.ThirdPartyPath("EA/EASTL/include"),
		LuminaConfig.ThirdPartyPath("EA/EABase/include/Common"),
	}

