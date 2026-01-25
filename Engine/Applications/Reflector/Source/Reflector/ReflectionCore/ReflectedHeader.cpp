#include "ReflectedHeader.h"
#include <filesystem>
#include "ReflectedProject.h"
#include "Reflector/ProjectSolution.h"

namespace Lumina::Reflection
{
    FReflectedHeader::FReflectedHeader(FReflectedProject* InProject, const eastl::string& Path)
        : HeaderPath(Path)
        , Project(InProject)
    {
        std::filesystem::path FilesystemPath = Path.c_str();
        FileName = FilesystemPath.stem().string().c_str();
        
        const eastl::string& WorkspacePath = InProject->Workspace->GetPath();
        eastl::string ProjectReflectionPath = WorkspacePath + "/Intermediates/Reflection/" + InProject->Name;
        eastl::string PossibleReflectedHeaderPath = ProjectReflectionPath + "/" + FileName + ".generated.h";
        
        bool bReflectionFileExists = std::filesystem::exists(PossibleReflectedHeaderPath.c_str());
        if (!bReflectionFileExists)
        {
            bDirty = true;
            return;
        }
        
        StartingFileTime = std::filesystem::last_write_time(FilesystemPath);
        auto LastReflectionWrite = std::filesystem::last_write_time(PossibleReflectedHeaderPath.c_str());
        
        if (StartingFileTime > LastReflectionWrite)
        {
            bDirty = true;
        }
    }
}
