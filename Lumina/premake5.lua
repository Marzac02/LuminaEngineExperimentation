
include "Scripts/Dependencies"

project "Lumina"
    kind "SharedLib"
    rtti "off"
    staticruntime "Off"
    enableunitybuild "On"
    pchheader "pch.h"
    pchsource "Engine/Source/pch.cpp"
    dependson { "Reflector", "EA", "ImGui", "Tracy", "GLFW" }
    enablereflection "true"

    defines
    {
        "LUMINA_ENGINE",
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
        LuminaConfig.GetReflectionUnityFile()
    }

    includedirs(LuminaConfig.GetPublicIncludeDirectories())
    
    libdirs
    {
        "%{LuminaConfig.EngineDirectory}/Lumina/Engine/ThirdParty/lua",
        "%{LuminaConfig.EngineDirectory}/Lumina/Engine/ThirdParty/NvidiaAftermath/lib",
        "%{LuminaConfig.EngineDirectory}/External/ShaderC",
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
        flags { "NoPCH" }
    filter {} -- reset

    -- Disable PCH and force C language for third-party C files
    filter "files:Engine/ThirdParty/**.c"
        flags { "NoPCH" }
        language "C"
    filter {} -- reset