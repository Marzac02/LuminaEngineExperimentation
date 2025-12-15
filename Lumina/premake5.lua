include(os.getenv("LUMINA_DIR") .. "/Dependencies.lua")

project "Lumina"
    kind "SharedLib"
    targetdir ("%{LuminaEngineDirectory}/Binaries/" .. outputdir)
    objdir ("%{LuminaEngineDirectory}/Intermediates/Obj/" .. outputdir .. "/%{prj.name}")

    pchheader "pch.h"
    pchsource "Engine/Source/pch.cpp"
    dependson { "GLFW", "EA", "ImGui", "Tracy", "Reflector" }

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
    }

    files
    {
        "Engine/Source/**.cpp",
        "Engine/Source/**.h",
        reflection_unity_file,
        
        "Engine/ThirdParty/EnkiTS/src/**.cpp",
        "Engine/ThirdParty/JoltPhysics/Jolt/**.cpp",
        "Engine/ThirdParty/xxhash/xxhash.c",
        "Engine/ThirdParty/rpmalloc/**.c",
        "Engine/ThirdParty/imgui/imgui_demo.cpp",
        "Engine/ThirdParty/imgui/implot_demo.cpp",
        "Engine/ThirdParty/meshoptimizer/src/**.cpp",
        "Engine/ThirdParty/vk-bootstrap/src/**.cpp",
        "Engine/ThirdParty/json/src/**.cpp",
        "Engine/ThirdParty/ImGuizmo/**.cpp",
        "Engine/ThirdParty/SPIRV-Reflect/**.c",
        "Engine/ThirdParty/SPIRV-Reflect/**.cpp",
        "Engine/ThirdParty/fastgltf/src/**.cpp",
        "Engine/ThirdParty/fastgltf/deps/simdjson/**.cpp",
        "Engine/ThirdParty/tinyobjloader/**.cc"
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
        "shaderc_combined",
        "GFSDK_Aftermath_Lib.x64"
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