#include "ReflectedStringProperty.h"


namespace Lumina
{
    bool FReflectedStringProperty::GenerateLuaBinding(eastl::string& Stream)
    {
        Stream += "\t\t\"" + GetDisplayName() + "\", "
        "sol::property([](" + Outer + "& Self) { return ""; })";

        return true;
    }
}