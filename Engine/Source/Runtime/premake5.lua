project "Runtime"
    kind "SharedLib"
    rtti "off"
    staticruntime "Off"
    pchheader "pch.h"
    pchsource "pch.cpp"
    dependson { "Reflector" }
    enablereflection "true"

    defines
    {
        "EASTL_USER_DEFINED_ALLOCATOR=1",
        "GLFW_INCLUDE_NONE",
        "GLFW_STATIC",
        "LUMINA_RENDERER_VULKAN",
        "VK_NO_PROTOTYPES",
        "LUMINA_RPMALLOC",
        "JPH_DEBUG_RENDERER",
        "JPH_FLOATING_POINT_EXCEPTIONS_ENABLED",
        "JPH_EXTERNAL_PROFILE",
        "JPH_ENABLE_ASSERTS",
    }

    files
    {
        "**.cpp",
        "**.h",
        "**.lua",
        LuminaConfig.GetReflectionFiles()
    }

    prebuildcommands 
    {
        LuminaConfig.RunReflection()
    }

    includedirs
    {
        LuminaConfig.GetPublicIncludeDirectories()
    }
    
    libdirs
    {
        "%{LuminaConfig.EngineDirectory}/Engine/Source/ThirdParty/NvidiaAftermath/lib",
        LuminaConfig.EnginePath("Engine/Source/ThirdParty/lua"),
        "%{LuminaConfig.EngineDirectory}/External/ShaderC",
    }

    fatalwarnings
    {
        "4456",
        "4457",
        "4458",
        "4238",
    }

    forceincludes
	{
		"LuminaAPI.h",
	}

    links
    {
        "GLFW",
        "ImGui",
        "EA",
        "Tracy",
        "lua54",
        "EnkiTS",
        "JoltPhysics",
        "RPMalloc",
        "XXHash",
        "Volk",
        "VKBootstrap",
        "TinyOBJLoader",
        "MeshOptimizer",
        "SPIRV-Reflect",
        "FastGLTF",
        "OpenFBX",
        "shaderc_combined",
        "GFSDK_Aftermath_Lib",
    }

    filter "configurations:Debug or configurations:DebugEditor"
        removelinks { "shaderc_combined" }
        links { "shaderc_combinedd" }
        
        
    filter "files:Engine/Source/ThirdParty/**.cpp"
        enablepch "Off"
    filter {}

    filter "files:Engine/Source/ThirdParty/**.c"
        enablepch "Off"
        language "C"
    filter {}