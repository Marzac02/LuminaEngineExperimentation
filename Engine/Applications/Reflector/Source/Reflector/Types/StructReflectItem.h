#pragma once
#include "EASTL/string.h"


namespace Lumina
{
    class IStructReflectable
    {
        virtual void GenerateMetadata(const eastl::string& InMetadata) = 0;
        virtual bool GenerateLuaBinding(eastl::string& Stream) {return false; }
    };
}
