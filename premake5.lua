
include "BuildScripts/Dependencies"
include "BuildScripts/Actions/Reflection"

workspace "Lumina"
	language "C++"
	targetdir "Build"
	conformancemode "On"
	cppdialect "C++latest"
	staticruntime "Off"
    warnings "Default"
    targetdir (LuminaConfig.GetTargetDirectory())
    objdir (LuminaConfig.GetObjDirectory())
    enableunitybuild "Off"
    fastuptodate "On"
    multiprocessorcompile "On"
	startproject "Editor"

    configurations 
    { 
        "Debug",
        "Development",
        "Shipping",
    }

    platforms
    {
        "Editor",
        "Game",
    }

    defaultplatform ("Editor")
		
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

	disablewarnings
    {
        "4251", -- DLL-interface warning
        "4275", -- Non-DLL-interface base class
        "4244", -- "Precision loss warnings"
        "4267", -- "Precision loss warnings"
	}

    filter "kind:SharedLib"
        defines { "%{prj.name:upper()}_EXPORTS"}
    filter {}

    filter "language:C++ or language:C"
		architecture "x86_64"
        
    filter "architecture:x86_64"
        defines { "LUMINA_PLATFORM_CPU_X86" }

    filter "system:windows"
        systemversion "latest"
        defines { "LE_PLATFORM_WINDOWS" }
        buildoptions 
        {
            "/Zc:preprocessor",
            "/Zc:inline",
            "/Zc:__cplusplus",
            "/bigobj"
        }

    filter {}
    
    filter "platforms:Game"
        defines { "WITH_EDITOR=0" }

    filter "platforms:Editor"
        defines { "WITH_EDITOR=1" }

    filter "configurations:Debug"
        linktimeoptimization "Off"
        incrementallink "On"
        optimize "Off"
        symbols "On"
        runtime "Debug"
        editandcontinue "On"
        defines { "LE_DEBUG", "LUMINA_DEBUG", "_DEBUG", "SOL_ALL_SAFETIES_ON", "DEBUG", }

    filter "configurations:Development"
        optimize "Speed"
        symbols "On"
        runtime "Release"
        linktimeoptimization "On"
        defines { "NDEBUG", "LE_DEVELOPMENT", "LUMINA_DEVELOPMENT", "SOL_ALL_SAFETIES_ON", }

    filter "configurations:Shipping"
        linktimeoptimization "On"
        optimize "Full"
        symbols "Off"
        runtime "Release"
        defines { "NDEBUG", "LE_SHIPPING", "LUMINA_SHIPPING" }
        removedefines { "TRACY_ENABLE" }
    
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

    group "Build"
        include "BuildScripts"
    group ""