
include "Scripts/Dependencies"
require "Scripts/Actions/Reflection"


workspace "Lumina"
	language "C++"
	targetdir "Build"
	startproject "Editor"
	conformancemode "On"
	cppdialect "C++latest"
	staticruntime "Off"
    warnings "Default"
    targetdir (LuminaConfig.GetTargetDirectory())
    objdir (LuminaConfig.GetObjDirectory())
    enableunitybuild "On"

    configurations 
    { 
        "Debug",
        "DebugEditor",
        "Development",
        "DevelopmentEditor",
        "Shipping",
    }

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

        "TRACY_ALLOW_SHADOW_WARNING",
        "TRACY_ENABLE",
    	"TRACY_CALLSTACK",
    	"TRACY_ON_DEMAND",
	}

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

    filter "language:C++ or language:C"
		architecture "x86_64"
        
    filter "architecture:x86_64"
        defines { "LUMINA_PLATFORM_CPU_X86" }

    filter "system:windows"
        systemversion "latest"
        defines { "LE_PLATFORM_WINDOWS" }
        buildoptions 
        { 
            "/EHsc",
            "/Zc:preprocessor",
            "/MP",
            "/Zc:inline",
            "/Zc:__cplusplus",
            "/we4238"
        }

    filter {}
    
    filter "configurations:Debug or configurations:Development or configurations:Shipping"
        defines { "WITH_EDITOR=NOT_IN_USE" }

    filter "configurations:DebugEditor or configurations:DevelopmentEditor"
        defines { "WITH_EDITOR=IN_USE" }

    filter "configurations:Debug or configurations:DebugEditor"
        optimize "Debug"
        symbols "On"
        runtime "Debug"
        editandcontinue "Off"
        defines { "LE_DEBUG", "LUMINA_DEBUG", "_DEBUG", "SOL_ALL_SAFETIES_ON", }
        flags { "NoRuntimeChecks" }
        linktimeoptimization "Off"

    filter "configurations:Development or configurations:DevelopmentEditor"
        optimize "Speed"
        symbols "On"
        runtime "Release"
        defines { "NDEBUG", "LE_DEVELOPMENT", "LUMINA_DEVELOPMENT", "SOL_ALL_SAFETIES_ON", }
        linktimeoptimization "on"

    filter "configurations:Shipping"
        optimize "Full"
        symbols "Off"
        runtime "Release"
        defines { "NDEBUG", "LE_SHIPPING", "LUMINA_SHIPPING" }
        removedefines { "TRACY_ENABLE" }
        linktimeoptimization "on"
    
    filter {}

    group "Engine"
		include "Lumina"
	group ""

	group "Applications"
		include "Lumina/Applications/LuminaEditor"
		include "Lumina/Applications/Reflector"
		include "Lumina/Applications/Sandbox"
	group ""

	group "ThirdParty"
        include "Lumina/Engine/ThirdParty/EA"
        include "Lumina/Engine/ThirdParty/EnTT"
		include "Lumina/Engine/ThirdParty/glfw"
		include "Lumina/Engine/ThirdParty/imgui"
		include "Lumina/Engine/Thirdparty/Tracy"
        include "Lumina/Engine/Thirdparty/EnkiTS"
        include "Lumina/Engine/ThirdParty/Sol2"
        include "Lumina/Engine/ThirdParty/JoltPhysics"
        include "Lumina/Engine/ThirdParty/RPMalloc"
        include "Lumina/Engine/ThirdParty/XXHash"
        include "Lumina/Engine/ThirdParty/VulkanMemoryAllocator"
        include "Lumina/Engine/ThirdParty/Volk"
        include "Lumina/Engine/ThirdParty/tinyobjloader"
        include "Lumina/Engine/ThirdParty/MeshOptimizer"
        include "Lumina/Engine/ThirdParty/vk-bootstrap"
        include "Lumina/Engine/ThirdParty/SPIRV-Reflect"
        include "Lumina/Engine/ThirdParty/json"
        include "Lumina/Engine/ThirdParty/fastgltf"
        include "Lumina/Engine/ThirdParty/OpenFBX"
	group ""