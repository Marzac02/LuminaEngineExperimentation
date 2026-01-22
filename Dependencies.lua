-- Validate required environment variables
LuminaEngineDirectory = os.getenv("LUMINA_DIR")

if not LuminaEngineDirectory then
    error("LUMINA_DIR environment variable not set. Run Setup.py first.")
end

-- Helper function for consistent paths
local function enginePath(subpath)
    return path.join(LuminaEngineDirectory, subpath)
end

-- Include directories
IncludeDir = {}
IncludeDir["Main"]                  = enginePath("Lumina/Engine/ThirdParty")
IncludeDir["spdlog"]                = enginePath("Lumina/Engine/ThirdParty/spdlog/include")
IncludeDir["glfw"]                  = enginePath("Lumina/Engine/ThirdParty/GLFW/include")
IncludeDir["glm"]                   = enginePath("Lumina/Engine/ThirdParty/GLM")
IncludeDir["imgui"]                 = enginePath("Lumina/Engine/ThirdParty/imgui")
IncludeDir["vkbootstrap"]           = enginePath("Lumina/Engine/ThirdParty/vk-bootstrap")
IncludeDir["VMA"]                   = enginePath("Lumina/Engine/ThirdParty/VulkanMemoryAllocator")
IncludeDir["fastgltf"]              = enginePath("Lumina/Engine/ThirdParty/fastgltf/include")
IncludeDir["OpenFBX"]               = enginePath("Lumina/Engine/ThirdParty/OpenFBX")
IncludeDir["stb"]                   = enginePath("Lumina/Engine/ThirdParty/stb_image")
IncludeDir["meshoptimizer"]         = enginePath("Lumina/Engine/ThirdParty/meshoptimizer/src")
IncludeDir["vulkan"]                = enginePath("Lumina/Engine/ThirdParty/vulkan")
IncludeDir["shaderc"]               = enginePath("Lumina/Engine/ThirdParty/shaderc")
IncludeDir["volk"]                  = enginePath("Lumina/Engine/ThirdParty/volk")
IncludeDir["EnkiTS"]                = enginePath("Lumina/Engine/ThirdParty/EnkiTS/src")
IncludeDir["SPIRV_Reflect"]         = enginePath("Lumina/Engine/ThirdParty/SPIRV-Reflect")
IncludeDir["json"]                  = enginePath("Lumina/Engine/ThirdParty/json")
IncludeDir["entt"]                  = enginePath("Lumina/Engine/ThirdParty/entt/single_include")
IncludeDir["EASTL"]                 = enginePath("Lumina/Engine/ThirdParty/EA/EASTL/include")
IncludeDir["EABase"]                = enginePath("Lumina/Engine/ThirdParty/EA/EABase/include/Common")
IncludeDir["rpmalloc"]              = enginePath("Lumina/Engine/ThirdParty/rpmalloc")
IncludeDir["xxhash"]                = enginePath("Lumina/Engine/ThirdParty/xxhash")
IncludeDir["tracy"]                 = enginePath("Lumina/Engine/ThirdParty/tracy/public")
IncludeDir["RenderDoc"]             = enginePath("Lumina/Engine/ThirdParty/RenderDoc")
IncludeDir["ConcurrentQueue"]       = enginePath("Lumina/Engine/ThirdParty/concurrentqueue")
IncludeDir["JoltPhysics"]           = enginePath("Lumina/Engine/ThirdParty/JoltPhysics")
IncludeDir["Sol2"]                  = enginePath("Lumina/Engine/ThirdParty/sol2/include")
IncludeDir["Lua"]                   = enginePath("Lumina/Engine/ThirdParty/lua/include")
IncludeDir["LuminaReflectDir"]      = path.join("%{wks.location}", "Intermediates/Reflection/Lumina")

-- Output directory pattern
outputdir               = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Reflection paths
reflection_dir          = path.join("%{wks.location}", "Intermediates/Reflection/%{prj.name}")
reflection_unity_file   = path.join(reflection_dir, "ReflectionUnity.gen.cpp")

-- Get all include directories as an array
function includedependencies()
    local includes = {}
    for _, dir in pairs(IncludeDir) do
        table.insert(includes, dir)
    end
    return includes
end