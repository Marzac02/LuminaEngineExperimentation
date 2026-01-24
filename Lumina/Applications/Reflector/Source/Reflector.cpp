
#include <chrono>
#include <fstream>
#include <print>
#include "StringHash.h"
#include "nlohmann/json.hpp"
#include "Reflector/ProjectSolution.h"
#include "Reflector/Clang/ClangParser.h"
#include "Reflector/CodeGeneration/CodeGenerator.h"
#include "Reflector/ReflectionCore/ReflectedProject.h"


using json = nlohmann::json;
using namespace Lumina::Reflection;

struct FScopeTimer
{
    using Clock = std::chrono::high_resolution_clock;

    eastl::string_view Name;
    Clock::time_point Start;

    explicit FScopeTimer(eastl::string_view InName)
        : Name(InName), Start(Clock::now())
    {}

    ~FScopeTimer()
    {
        auto End = Clock::now();
        auto S  = std::chrono::duration_cast<std::chrono::seconds>(End - Start).count();

        std::println("[PROFILE] {} {} s", Name.data(), S);
    }
};

int main(int argc, char* argv[])
{
    FScopeTimer TotalTimer("Total Execution");
    
    Lumina::FStringHash::Initialize();
    
    std::println("===============================================");
    std::println("======== Lumina Reflection Tool (LRT) =========");
    std::println("===============================================");
    
#if 0
    
    eastl::string InputFile = "H:/LuminaEngine/Reflection_Files.json";
    
#else
    if (argc < 2)
    {
        std::println("Missing command line argument");
        return 1;
    }
    
    eastl::string InputFile = argv[1];
#endif
    
    std::ifstream File(InputFile.c_str());
    if (!File.is_open())
    {
        std::println("Failed to open file {}", InputFile.c_str());
        return 1; 
    }
    
    
    json Data = json::parse(File);
    
    eastl::string WorkspaceName     = Data["WorkspaceName"].get<std::string>().c_str();
    eastl::string WorkspacePath     = Data["WorkspacePath"].get<std::string>().c_str();
    
    FReflectedWorkspace Workspace(WorkspacePath.c_str());
    
    bool bAnyHeaderDirty = false;
    
    for (const auto& Project : Data["Projects"])
    {
        eastl::string ProjectName = Project["Name"].get<std::string>().c_str();
        
        auto ReflectedProject = eastl::make_unique<FReflectedProject>(&Workspace);
        ReflectedProject->Name = eastl::move(ProjectName);
        
        for (const auto& IncludeDirJson : Project["IncludeDirs"])
        {
            eastl::string IncludeDir = IncludeDirJson.get<std::string>().c_str();
            ReflectedProject->IncludeDirs.push_back(eastl::move(IncludeDir));
        }
        
        for (const auto& ProjectFileJson : Project["Files"])
        {
            eastl::string ProjectFile = ProjectFileJson.get<std::string>().c_str();
            ProjectFile.make_lower();
            eastl::replace(ProjectFile.begin(), ProjectFile.end(), '\\', '/');
            
            auto ReflectedHeader = eastl::make_unique<FReflectedHeader>(ReflectedProject.get(), ProjectFile);
            if (ReflectedHeader->bDirty)
            {
                bAnyHeaderDirty = true;
            }
            
            Lumina::FStringHash HeaderHash(ProjectFile);
            ReflectedProject->Headers.emplace(HeaderHash, eastl::move(ReflectedHeader));
        }
        
        Workspace.AddReflectedProject(eastl::move(ReflectedProject));
    }
    
    if (!bAnyHeaderDirty)
    {
        std::println("Reflection not necessary");
        return 0;
    }
    
    FClangParser Parser;
    Parser.Parse(&Workspace);
    
    FCodeGenerator CodeGenerator(&Workspace, Parser.ParsingContext.ReflectionDatabase);
    
    for (eastl::unique_ptr<FReflectedProject>& Project : Workspace.ReflectedProjects)
    {
        CodeGenerator.GenerateCodeForProject(Project.get());
    }

    for (auto& Project : Workspace.ReflectedProjects)
    {
        for (auto& Header : Project->Headers)
        {
            try
            {
                std::filesystem::last_write_time(Header.second->HeaderPath.c_str(), Header.second->StartingFileTime);
            }
            catch (std::filesystem::filesystem_error& Error)
            {
                std::println("Failed to set last write time: {}", Error.what());
            }
        }
    }

    Lumina::FStringHash::Shutdown();
    
    return 0;
}
