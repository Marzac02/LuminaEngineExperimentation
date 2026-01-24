#include "ClangParserContext.h"

#include <iostream>

#include "Utils.h"
#include "xxhash.h"
#include "EASTL/queue.h"

namespace Lumina::Reflection
{
    void FClangParserContext::AddReflectedMacro(FReflectionMacro&& Macro)
    {
        uint64_t Hash = XXH64(Macro.HeaderID.c_str(), strlen(Macro.HeaderID.c_str()), 0);

        eastl::vector<FReflectionMacro>& Macros = ReflectionMacros[Hash];
        Macros.push_back(eastl::move(Macro));
    }

    void FClangParserContext::AddGeneratedBodyMacro(FReflectionMacro&& Macro)
    {
        uint64_t Hash = ClangUtils::HashString(Macro.HeaderID);
        
        eastl::queue<FReflectionMacro>& Macros = GeneratedBodyMacros[Hash];
        Macros.push(eastl::move(Macro));
    }

    bool FClangParserContext::TryFindMacroForCursor(const eastl::string& HeaderID, const CXCursor& Cursor, FReflectionMacro& Macro)
    {
        uint64_t Hash = ClangUtils::HashString(HeaderID);

        auto HeaderIter = ReflectionMacros.find(Hash);
        if (HeaderIter == ReflectionMacros.end())
        {
            return false;
        }

        CXSourceRange typeRange = clang_getCursorExtent(Cursor);
        CXSourceLocation startLoc = clang_getRangeStart(typeRange);

        CXFile cursorFile;
        uint32_t cursorLine, cursorColumn;
        clang_getExpansionLocation(startLoc, &cursorFile, &cursorLine, &cursorColumn, nullptr);

        CXString FileName = clang_getFileName(cursorFile);

        if (FileName.data == nullptr)
        {
            return false;
        }

        eastl::string FileNameChar = clang_getCString(FileName);
        FileNameChar.make_lower();
        eastl::replace(FileNameChar.begin(), FileNameChar.end(), '\\', '/');

        clang_disposeString(FileName);

        if (FileNameChar != HeaderID)
        {
            return false;
        }

        eastl::vector<FReflectionMacro>& MacrosForHeader = HeaderIter->second;
        for (auto iter = MacrosForHeader.begin(); iter != MacrosForHeader.end(); ++iter)
        {
            bool bValidMacro = (iter->LineNumber < cursorLine) && ((cursorLine - iter->LineNumber) <= 1);
        
            if (bValidMacro)
            {
                Macro = *iter;
                MacrosForHeader.erase(iter);
                return true;
            }
        }
        
        return false;
    }

    bool FClangParserContext::TryFindGeneratedBodyMacro(const eastl::string& HeaderID, const CXCursor& Cursor, FReflectionMacro& Macro)
    {
        // Exported types, we don't care.
        if (HeaderID.find("manualreflecttypes") != eastl::string::npos)
        {
            return true;
        }
        
        uint64_t Hash = XXH64(HeaderID.c_str(), strlen(HeaderID.c_str()), 0);
        auto headerIter = GeneratedBodyMacros.find(Hash);
        if (headerIter == GeneratedBodyMacros.end())
        {
            Macro = {};
            return false;
        }

        
        eastl::queue<FReflectionMacro>& MacrosForHeader = headerIter->second;

        if (MacrosForHeader.empty())
        {
            Macro = {};
            return false;
        }
        
        Macro = MacrosForHeader.front();
        
        MacrosForHeader.pop();

        return true;
    }
    
    void FClangParserContext::PushNamespace(const eastl::string& Namespace)
    {
        NamespaceStack.push_back(Namespace);

        CurrentNamespace.clear();
        for (const eastl::string& String : NamespaceStack)
        {
            CurrentNamespace.append(String);
        }
    }

    void FClangParserContext::PopNamespace()
    {
        NamespaceStack.pop_back();

        CurrentNamespace.clear();
        for (const eastl::string& String : NamespaceStack)
        {
            CurrentNamespace.append(String);
        }
    }
}
