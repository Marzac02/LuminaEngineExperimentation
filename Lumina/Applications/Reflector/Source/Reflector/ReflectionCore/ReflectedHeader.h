#pragma once
#include "EASTL/string.h"


namespace Lumina::Reflection
{
    class FReflectedProject;

    class FReflectedHeader
    {
    public:
        
        FReflectedHeader(FReflectedProject* InProject, const eastl::string& Path);
        
        eastl::string               FileName;
        eastl::string               HeaderPath;
        FReflectedProject*          Project;
        
        bool                        bDirty = false;
    };
}
