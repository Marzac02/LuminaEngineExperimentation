#include "ReflectedObjectProperty.h"

namespace Lumina
{
    bool FReflectedObjectProperty::GenerateLuaBinding(eastl::string& Stream)
    {
        Stream += "\t\t\"" + GetDisplayName() + "\", "
        "sol::property([](" + Outer + "& Self) { return nullptr; })";//\n"
        //"\t\t[](" + Outer + "& Self, " + TypeName + "* Obj) { Self." + Name + " = Obj; })";

        return true;
    }
}
