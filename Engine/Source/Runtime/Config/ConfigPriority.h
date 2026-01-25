#pragma once
#include "Containers/String.h"


namespace Lumina
{
    
    struct FConfigEntry
    {
        const FStringView Name;
        const FStringView Path;
    };
    
    inline FConfigEntry GConfigEntries[] =
    {
        { "Base",                      "{ENGINE}/Config/Base.json" },
        { "ProjectDefault",            "{PROJECT}/Config/Default{TYPE}.json" }
    };
    
    
}
