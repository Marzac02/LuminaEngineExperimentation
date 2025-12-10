#include "ProjectSolution.h"
#include "ReflectionCore/ReflectedProject.h"

namespace Lumina::Reflection
{
    FProjectSolution::FProjectSolution(const std::filesystem::path& ReflectionPath)
        : Path(ReflectionPath.string().c_str())
        , ParentPath(ReflectionPath.parent_path().string().c_str())
    {
    }

    void FProjectSolution::AddReflectedProject(eastl::shared_ptr<FReflectedProject>&& Project)
    {
        ReflectedProjects.push_back(eastl::move(Project));
    }
}
