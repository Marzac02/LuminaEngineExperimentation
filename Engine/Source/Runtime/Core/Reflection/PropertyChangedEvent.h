#pragma once
#include "Containers/Name.h"


namespace Lumina
{
    class CStruct;
    class FProperty;
}

namespace Lumina
{

    struct FPropertyChangedEvent
    {
        /** Type that owns the property */
        CStruct*    OuterType;
        
        /** Property in which was changed. */
        FProperty*  Property;

        /** Name of the property changed */
        FName       PropertyName;
        
    };

    
}
