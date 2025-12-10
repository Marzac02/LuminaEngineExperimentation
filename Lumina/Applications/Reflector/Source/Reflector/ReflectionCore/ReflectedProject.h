#pragma once
#include "ReflectedHeader.h"
#include "StringHash.h"

#include "EASTL/hash_map.h"
#include "eastl/shared_ptr.h"
#include "Reflector/Clang/Visitors/ClangTranslationUnit.h"

namespace Lumina::Reflection
{
    class FReflectedProject
    {
    public:

        FReflectedProject(const eastl::string& SlnPath, const eastl::string& ProjectPath);

        bool Parse();
        
        eastl::string                                                       Name;
        eastl::string                                                       Path;
        eastl::string                                                       SolutionPath;
        eastl::string                                                       ParentPath;
        eastl::vector<eastl::shared_ptr<FReflectedHeader>>                  Headers;
        eastl::hash_map<FStringHash, eastl::shared_ptr<FReflectedHeader>>   HeaderHashMap;
        CXTranslationUnit                                                   TranslationUnit = nullptr;
        CXIndex                                                             ClangIndex = nullptr;
        bool                                                                bDirty = false;
        
    };
}
