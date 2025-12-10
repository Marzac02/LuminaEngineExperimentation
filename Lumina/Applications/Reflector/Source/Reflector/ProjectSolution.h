#pragma once

#include <filesystem>
#include "eastl/shared_ptr.h"
#include "EASTL/string.h"
#include "EASTL/vector.h"

namespace Lumina::Reflection
{
    class FReflectedProject;

    class FProjectSolution
    {
    public:

        FProjectSolution(const std::filesystem::path& ReflectionPath);

        
        const eastl::string& GetPath() const { return Path; }
        const eastl::string& GetParentPath() const { return ParentPath; }

        void AddReflectedProject(eastl::shared_ptr<FReflectedProject>&& Project);
        bool HasProjects() const { return !ReflectedProjects.empty(); }
        const eastl::vector<eastl::shared_ptr<FReflectedProject>>& GetProjects() const { return ReflectedProjects; }
        
    private:

        eastl::string Path;
        eastl::string ParentPath;
        eastl::vector<eastl::shared_ptr<FReflectedProject>> ReflectedProjects;
        
    };
}
