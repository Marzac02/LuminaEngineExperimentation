#pragma once
#include <filesystem>

#include "EASTL/string.h"


namespace Lumina::Reflection
{
    class FReflectedProject;

    class FReflectedHeader
    {
    public:
        
        FReflectedHeader(FReflectedProject* InProject, const eastl::string& Path);
        
        std::filesystem::file_time_type StartingFileTime;
        eastl::string                   FileName;
        eastl::string                   HeaderPath;
        FReflectedProject*              Project;
                                        
        bool                            bDirty = false;
    };
}
