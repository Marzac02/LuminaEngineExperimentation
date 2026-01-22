LuminaConfig = LuminaConfig or {}
LuminaConfig.PublicIncludes         = LuminaConfig.PublicIncludes or {}


LuminaConfig.EngineDirectory        = os.getenv("LUMINA_DIR")
LuminaConfig.OutputDirectory        = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
LuminaConfig.ProjectFilesDirectory  = "%{wks.location}/Intermediates/ProjectFiles/%{prj.name}"
LuminaConfig.ReflectionDirectory    = "%{wks.location}/Intermediates/Reflection/%{prj.name}"

if not LuminaConfig.EngineDirectory then
    error("LUMINA_DIR environment variable not set. Run Setup.py first")
end

function LuminaConfig.GetTargetDirectory()
    return path.join(LuminaConfig.EngineDirectory, "Binaries", LuminaConfig.OutputDirectory)
end

function LuminaConfig.GetProjectName()
    return "%{prj.name}"
end

function LuminaConfig.GetObjDirectory()
    return path.join(LuminaConfig.EngineDirectory, "Intermediates", "Obj", LuminaConfig.OutputDirectory, LuminaConfig.GetProjectName())
end

function LuminaConfig.ThirdPartyDirectory()
    return path.join(LuminaConfig.EngineDirectory, "Lumina/Engine/ThirdParty")
end

function LuminaConfig.EnginePath(Subpath)
    return path.join(LuminaConfig.EngineDirectory, Subpath)
end

function LuminaConfig.ThirdPartyPath(Subpath)
    return path.join(LuminaConfig.EngineDirectory, "Lumina/Engine/ThirdParty", Subpath)
end

function LuminaConfig.Execute(Command, ...)
    local args = {...}
    local quote = function(s) return "\"" .. s .. "\"" end

    for i, v in ipairs(args) do
        args[i] = quote(v)
    end

    local cmd = Command .. " " .. table.concat(args, " ")

    table.insert(LuminaConfig.PreBuildCommands, cmd)

    return cmd
end

function LuminaConfig.WorkspacePath(Subpath)
    return path.join("%{wks.location}", Subpath)
end

function LuminaConfig.ProjectPath(Subpath)
    return path.join("%{prj.location}", Subpath)
end

function LuminaConfig.GetReflectionUnityFile()
    return path.join(LuminaConfig.ReflectionDirectory, "ReflectionUnity.gen.cpp")
end

function LuminaConfig.AddPublicIncludeDirectory(Path)
    table.insert(LuminaConfig.PublicIncludes, Path)
end

function LuminaConfig.GetPublicIncludeDirectories()
    return LuminaConfig.PublicIncludes
end

function LuminaConfig.CopyFile(Source, Destination)
    if not Source or not Destination then
        error("CopyFile requires source and Destination")
    end

    local Quote =  function(s) return "\"" .. s .. "\"" end

    return "{COPYFILE} " .. Quote(Source) .. " " .. Quote(Destination)
end

function LuminaConfig.MakeDirectory(Path)

    local Quote =  function(s) return "\"" .. s .. "\"" end
    return "{MKDIR} " .. Quote(Path)
end

function LuminaConfig.RunReflection(Path)

    local ReflectorEXE = path.join(LuminaConfig.EngineDirectory, "Binaries", LuminaConfig.OutputDirectory, "Reflector.exe")

    return string.format('"%s" "%s"', ReflectorEXE, Path)

end

LuminaConfig.AddPublicIncludeDirectory("Source")
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/Source"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/Source/Runtime"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Intermediates/Reflection/Lumina"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.ReflectionDirectory)
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/ThirdParty"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/ThirdParty/spdlog/include"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/ThirdParty/GLFW/include"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/ThirdParty/GLM"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/ThirdParty/imgui"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/ThirdParty/vk-bootstrap"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/ThirdParty/VulkanMemoryAllocator"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/ThirdParty/fastgltf/include"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/ThirdParty/OpenFBX"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/ThirdParty/stb_image"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/ThirdParty/meshoptimizer/src"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/ThirdParty/vulkan"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/ThirdParty/shaderc"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/ThirdParty/volk"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/ThirdParty/EnkiTS/src"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/ThirdParty/SPIRV-Reflect"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/ThirdParty/json"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/ThirdParty/entt/single_include"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/ThirdParty/EA/EASTL/include"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/ThirdParty/EA/EABase/include/Common"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/ThirdParty/rpmalloc"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/ThirdParty/xxhash"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/ThirdParty/tracy/public"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/ThirdParty/RenderDoc"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/ThirdParty/concurrentqueue"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/ThirdParty/JoltPhysics"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/ThirdParty/sol2/include"))
LuminaConfig.AddPublicIncludeDirectory(LuminaConfig.EnginePath("Lumina/Engine/ThirdParty/lua/include"))