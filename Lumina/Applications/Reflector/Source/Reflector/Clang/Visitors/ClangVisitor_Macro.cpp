#include "ClangVisitor.h"
#include "eastl/fixed_hash_map.h"

#include "Reflector/Clang/ClangParserContext.h"
#include "Reflector/Clang/Utils.h"

namespace Lumina::Reflection::Visitor
{

    static const eastl::fixed_hash_map<eastl::string, EReflectionMacro, 8> MacroMap =
    {
        { ReflectionEnumToString(EReflectionMacro::Property),      EReflectionMacro::Property },
        { ReflectionEnumToString(EReflectionMacro::Function),      EReflectionMacro::Function },
        { ReflectionEnumToString(EReflectionMacro::Reflect),       EReflectionMacro::Reflect  },
        { ReflectionEnumToString(EReflectionMacro::GeneratedBody), EReflectionMacro::GeneratedBody }
    };
    
    CXChildVisitResult VisitMacro(const CXCursor& Cursor, CXCursor, FClangParserContext* Context)
    {
        eastl::string CursorName = ClangUtils::GetCursorDisplayName(Cursor);
        CXSourceRange Range = clang_getCursorExtent(Cursor);

        auto It = MacroMap.find(CursorName);
        if (It != MacroMap.end())
        {
            FReflectionMacro Macro(Context->ReflectedHeader->HeaderPath, Cursor, Range, It->second);
            if (It->second == EReflectionMacro::GeneratedBody)
            {
                Context->AddGeneratedBodyMacro(std::move(Macro));
            }
            else
            {
                Context->AddReflectedMacro(std::move(Macro));
            }
        }
        
        return CXChildVisit_Continue;

    }
}
