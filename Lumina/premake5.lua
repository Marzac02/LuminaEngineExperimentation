
include "Scripts/Dependencies"

project "Lumina"
    kind "SharedLib"
    rtti "off"
    staticruntime "Off"
    pchheader "pch.h"
    pchsource "Engine/Source/pch.cpp"
    dependson { "Reflector", "EA", "ImGui", "Tracy", "GLFW" }
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
        "Engine/Source/**.cpp",
        "Engine/Source/**.h",
        "Premake5.lua",
        LuminaConfig.GetReflectionUnityFile()
    }

    includedirs(LuminaConfig.GetPublicIncludeDirectories())
    
    libdirs
    {
        "%{LuminaConfig.EngineDirectory}/Lumina/Engine/ThirdParty/lua",
        "%{LuminaConfig.EngineDirectory}/Lumina/Engine/ThirdParty/NvidiaAftermath/lib",
        "%{LuminaConfig.EngineDirectory}/External/ShaderC",
    }

    fatalwarnings
    {
        "4456",
        "4457",
        "4458",
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

    prebuildcommands 
    {
        LuminaConfig.RunReflection()
    }
    
    postbuildcommands
    {
        '{COPYFILE} "%{LuminaConfig.EngineDirectory}/External/RenderDoc/renderdoc.dll" "%{cfg.targetdir}"',
        '{COPYFILE} "%{LuminaConfig.EngineDirectory}/Lumina/Engine/ThirdParty/NvidiaAftermath/lib/GFSDK_Aftermath_Lib.x64.dll" "%{cfg.targetdir}"',
    }

    filter "configurations:Debug or configurations:DebugEditor"
        removelinks { "shaderc_combined" }
        links { "shaderc_combinedd" }
        
        
    filter "files:Engine/ThirdParty/**.cpp"
        enablepch "Off"
    filter {}

    filter "files:Engine/ThirdParty/**.c"
        enablepch "Off"
        language "C"
    filter {}