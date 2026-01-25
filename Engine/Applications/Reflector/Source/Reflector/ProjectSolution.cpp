#include "ProjectSolution.h"
#include "ReflectionCore/ReflectedProject.h"

namespace Lumina::Reflection
{
    FReflectedWorkspace::FReflectedWorkspace(const std::filesystem::path& ReflectionPath)
        : Path(ReflectionPath.string().c_str())
    {
    }

    void FReflectedWorkspace::AddReflectedProject(eastl::unique_ptr<FReflectedProject>&& Project)
    {
        ReflectedProjects.push_back(eastl::move(Project));
    }
}
