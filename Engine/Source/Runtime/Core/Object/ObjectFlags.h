#pragma once
#include "Containers/String.h"
#include "Core/LuminaMacros.h"


namespace Lumina
{
    /** Core object flags for all CObjects, describes it's state. */
    enum EObjectFlags
    {
        /** NULL */
        OF_None                 = 0,

        /** Should not be saved */
        OF_Transient            = BIT(0),

        /** This object is currently a part of the root set */
        OF_Rooted               = BIT(1),
        
        /** Is this a default subobject of a class? */
        OF_DefaultObject        = BIT(2),

        /** Does this object need to be loaded after creation? */
        OF_NeedsLoad            = BIT(3),
        
        /** Is this object currently being loaded? */
        OF_Loading              = BIT(4),

        /** Does this object need PostLoad called? This will not deserialize the object again. */
        OF_NeedsPostLoad        = BIT(5),

        /** Was this object loaded from a package */
        OF_WasLoaded            = BIT(6),

        /** Object is public outside of it's package (assets and such) */
        OF_Public               = BIT(7),

        /** Object has already been marked to be destroyed */
        OF_MarkedDestroy        = BIT(8),
    };

    ENUM_CLASS_FLAGS(EObjectFlags);

    inline FFixedString ObjectFlagsToString(EObjectFlags Flags)
    {
        FFixedString Result;

        if (Flags == OF_None)
        {
            return "None";
        }

        if (EnumHasAnyFlags(Flags, OF_Transient))       Result += "OF_Transient|";
        if (EnumHasAnyFlags(Flags, OF_Rooted))          Result += "OF_Rooted|";
        if (EnumHasAnyFlags(Flags, OF_DefaultObject))   Result += "OF_DefaultObject|";
        if (EnumHasAnyFlags(Flags, OF_NeedsLoad))       Result += "OF_NeedsLoad|";
        if (EnumHasAnyFlags(Flags, OF_NeedsLoad))       Result += "OF_NeedsPostLoad|";
        if (EnumHasAnyFlags(Flags, OF_WasLoaded))       Result += "OF_WasLoaded|";
        if (EnumHasAnyFlags(Flags, OF_Public))          Result += "OF_Public|";
        if (EnumHasAnyFlags(Flags, OF_MarkedDestroy))   Result += "OF_MarkedDestroy|";

        if (!Result.empty() && Result.back() == '|') Result.pop_back();

        return Result;
    }
}
