workspace "Lumina"
	configurations { "Debug", "Development", "Shipping" }
	targetdir "Build"
	startproject "Editor"
	conformancemode "On"

	language "C++"
	cppdialect "C++latest"
	staticruntime "Off"

	flags  
	{
		"MultiProcessorCompile",
        "NoIncrementalLink",
        "ShadowedVariables",
	}
		
	defines
	{
		"EASTL_USER_DEFINED_ALLOCATOR=1",
		"_CRT_SECURE_NO_WARNINGS",
        "_SILENCE_CXX23_ALIGNED_UNION_DEPRECATION_WARNING",
        "_SILENCE_CXX23_ALIGNED_STORAGE_DEPRECATION_WARNING",
		"GLM_FORCE_DEPTH_ZERO_TO_ONE",
		"GLM_FORCE_LEFT_HANDED",
        "GLM_ENABLE_EXPERIMENTAL",
		"IMGUI_DEFINE_MATH_OPERATORS",
        "IMGUI_IMPL_VULKAN_USE_VOLK",
        "SOL_DEFAULT_PASS_ON_ERROR",

        "TRACY_ALLOW_SHADOW_WARNING",
        "TRACY_ENABLE",
    	"TRACY_CALLSTACK",
    	"TRACY_ON_DEMAND",
	}

	filter "action:vs"
	
	filter "language:C++ or language:C"
		architecture "x86_64"

        
    filter "architecture:x86_64"
        defines { "LUMINA_PLATFORM_CPU_X86" }

	buildoptions 
    {
		"/Zm2000",
        "/bigobj"
	}

	disablewarnings
    {
        "4251", -- DLL-interface warning
        "4275", -- Non-DLL-interface base class
        "4244", -- "Precision loss warnings"
        "4267", -- "Precision loss warnings"
	}

    warnings "Default"

    -- Platform-specific settings
    filter "system:windows"
        systemversion "latest"
        defines { "LE_PLATFORM_WINDOWS" }
        buildoptions 
        { 
            "/EHsc",
            "/Zc:preprocessor",
            "/MP",      -- Multi-processor compilation
            "/Zc:inline", -- Remove unreferenced functions/data
            "/Zc:__cplusplus",
            "/we4238"
        }

    filter "system:linux"
        defines { "LE_PLATFORM_LINUX" }
        buildoptions { "-march=native" } -- CPU-specific optimizations

    -- Debug Configuration
    filter "configurations:Debug"
        optimize "Debug"
        symbols "On"
        editandcontinue "Off"
        defines { "LE_DEBUG", "LUMINA_DEBUG", "_DEBUG", "JPH_DEBUG", "SOL_ALL_SAFETIES_ON", }
        flags { "NoRuntimeChecks" }
        linktimeoptimization "Off"


    -- Release Configuration (Developer build with symbols)
    filter "configurations:Development"
        optimize "Speed"
        symbols "On"
        defines { "LE_RELEASE", "LUMINA_DEVELOPMENT", "NDEBUG", "LUMINA_DEVELOPMENT", "SOL_ALL_SAFETIES_ON", }
        linktimeoptimization "on"



    -- Shipping Configuration (Maximum optimization, no symbols)
    filter "configurations:Shipping"
        optimize "Full"
        symbols "Off"
        defines { "LE_SHIP", "LUMINA_SHIPPING", "NDEBUG" }
        removedefines { "TRACY_ENABLE" }
        linktimeoptimization "on"
        

    filter {}

	group "Dependencies"
		include "Lumina/Engine/ThirdParty/EA"
		include "Lumina/Engine/ThirdParty/glfw"
		include "Lumina/Engine/ThirdParty/imgui"
		include "Lumina/Engine/Thirdparty/Tracy"
	group ""

	group "Core"
		include "Lumina"
	group ""

	group "Applications"
		include "Lumina/Applications/LuminaEditor"
		include "Lumina/Applications/Reflector"
		include "Lumina/Applications/Sandbox"
	group ""