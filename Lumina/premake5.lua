include(os.getenv("LUMINA_DIR") .. "/Dependencies.lua")

project "Lumina"
    kind "SharedLib"
    rtti "off"
    staticruntime "Off"
    enableunitybuild "On"
    targetdir ("%{LuminaEngineDirectory}/Binaries/" .. outputdir)
    objdir ("%{LuminaEngineDirectory}/Intermediates/Obj/" .. outputdir .. "/%{prj.name}")

    pchheader "pch.h"
    pchsource "Engine/Source/pch.cpp"
    dependson { "EA", "ImGui", "Tracy", "GLFW", "Reflector" }


    defines
    {
        "LUMINA_ENGINE_DIRECTORY=%{LuminaEngineDirectory}",
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

    prebuildcommands 
    {
        'python "%{LuminaEngineDirectory}/Scripts/RunReflection.py" "%{wks.location}/Lumina.sln"'
    }
    prebuildmessage "======== Running Lumina Reflection Tool ========"

    postbuildcommands
    {
        '{COPYFILE} "%{LuminaEngineDirectory}/External/RenderDoc/renderdoc.dll" "%{cfg.targetdir}"',
        '{COPYFILE} "%{LuminaEngineDirectory}/Lumina/Engine/ThirdParty/NvidiaAftermath/lib/GFSDK_Aftermath_Lib.x64.dll" "%{cfg.targetdir}"',
    }

    files
    {
        "Engine/Source/**.cpp",
        "Engine/Source/**.h",
        reflection_unity_file,
    }

    includedirs
    { 
        "Engine/Source",
        "Engine/Source/Runtime",
        
        reflection_dir,
        includedependencies(),
    }
    
    libdirs
    {
        "%{LuminaEngineDirectory}/Lumina/Engine/ThirdParty/lua",
        "%{LuminaEngineDirectory}/Lumina/Engine/ThirdParty/NvidiaAftermath/lib",
        "%{LuminaEngineDirectory}/External/ShaderC",
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

    filter "configurations:Debug"
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