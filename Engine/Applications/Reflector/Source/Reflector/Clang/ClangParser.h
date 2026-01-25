#pragma once
#include "ClangParserContext.h"

namespace Lumina::Reflection
{
    class FClangParser
    {
    public:

        FClangParser() = default;

        bool Parse(FReflectedWorkspace* Workspace);
        
        FClangParserContext ParsingContext;
        
    };
}
