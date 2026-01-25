#include "ClangParser.h"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <fstream>
#include <print>
#include <clang-c/Index.h>
#include "EASTL/fixed_vector.h"
#include "Reflector/ProjectSolution.h"
#include "Reflector/ReflectionCore/ReflectedProject.h"
#include "Visitors/ClangTranslationUnit.h"



namespace Lumina::Reflection
{
    bool FClangParser::Parse(FReflectedWorkspace* Workspace)
    {
        CXTranslationUnit TranslationUnit = nullptr;
        CXIndex ClangIndex = nullptr;
    
        ParsingContext.Workspace = Workspace;
        
        const eastl::string AmalgamationPath = std::filesystem::absolute("ReflectHeaders.gen.h").string().c_str();

        std::ofstream AmalgamationFile(AmalgamationPath.c_str());
        if (!AmalgamationFile.is_open())
        {
            spdlog::error("Failed to create amalgamation file");
            return false;
        }
        AmalgamationFile << "#pragma once\n\n";
        
        eastl::vector<eastl::string> FullIncludePaths;
        eastl::fixed_vector<const char*, 32> ClangArgs;
        
        eastl::string LuminaDirectory = std::getenv("LUMINA_DIR");
        if (!LuminaDirectory.empty() && LuminaDirectory.back() == '/' )
        {
            LuminaDirectory.pop_back();
        }
        
        for (const auto& Project : Workspace->ReflectedProjects)
        {
            for (const eastl::string& IncludeDir : Project->IncludeDirs)
            {
                if (IncludeDir.find("GLM") != eastl::string::npos || IncludeDir.find("glm") != eastl::string::npos)
                {
                    continue;
                }
                
                ClangArgs.emplace_back("-I");
                ClangArgs.emplace_back(IncludeDir.c_str());
            }
            
            for (auto& [Path, Header] : Project->Headers)
            {
                AmalgamationFile << "#include \"" << Path.c_str() << "\"\n";
                ParsingContext.AllHeaders.emplace(Path, Header.get());
                // @TODO For some reason enabling this breaks the manualreflecttypes.
                //ClangArgs.emplace_back("-include");
                //ClangArgs.emplace_back(Path.c_str());
                ParsingContext.NumHeadersReflected++;
            }
        }
    
        AmalgamationFile.close();   
        
        eastl::string ManualReflectPath;
        ManualReflectPath.append(LuminaDirectory).append("/Lumina/Engine/Source/Runtime/Core/Object/ManualReflectTypes.h");
        ClangArgs.emplace_back("-include");
        ClangArgs.emplace_back(ManualReflectPath.c_str());
        
        ClangArgs.emplace_back("-x");
        ClangArgs.emplace_back("c++");
        ClangArgs.emplace_back("-std=c++23");
        ClangArgs.emplace_back("-O0");
        ClangArgs.emplace_back("-D");
        ClangArgs.emplace_back("REFLECTION_PARSER");
        ClangArgs.emplace_back("-D");
        ClangArgs.emplace_back("NDEBUG");
        ClangArgs.emplace_back("-DRUNTIME_API=");
        ClangArgs.emplace_back("-fms-extensions");
        ClangArgs.emplace_back("-fms-compatibility");
        ClangArgs.emplace_back("-Wfatal-errors=0");
        ClangArgs.emplace_back("-w");
        ClangArgs.emplace_back("-ferror-limit=1000000000");
        ClangArgs.emplace_back("-Wno-multichar");
        ClangArgs.emplace_back("-Wno-deprecated-builtins");
        ClangArgs.emplace_back("-Wno-unknown-warning-option");
        ClangArgs.emplace_back("-Wno-return-type-c-linkage");
        ClangArgs.emplace_back("-Wno-c++98-compat-pedantic");
        ClangArgs.emplace_back("-Wno-gnu-folding-constant");
        ClangArgs.emplace_back("-Wno-vla-extension-static-assert");
        ClangArgs.emplace_back("-fno-spell-checking");
        ClangArgs.emplace_back("-fno-delayed-template-parsing");
    
        ClangIndex = clang_createIndex(0, 0);
        
        constexpr uint32_t ClangOptions = 
            CXTranslationUnit_DetailedPreprocessingRecord |
            CXTranslationUnit_SkipFunctionBodies | 
            CXTranslationUnit_KeepGoing;
        
        CXErrorCode Result = clang_parseTranslationUnit2(
            ClangIndex,
            AmalgamationPath.c_str(),
            ClangArgs.data(),
            (int)ClangArgs.size(),
            nullptr,
            0,
            ClangOptions,
            &TranslationUnit);
        
        CXCursor Cursor = clang_getTranslationUnitCursor(TranslationUnit);
        if (clang_visitChildren(Cursor, VisitTranslationUnit, &ParsingContext) != 0)
        {
            spdlog::error("A problem occured during translation unit parsing");
        }
        
        if (Result != CXError_Success)
        {
            switch (Result)
            {
            case CXError_Failure:
                spdlog::error("Clang Unknown failure");
                break;
    
            case CXError_Crashed:
                spdlog::error("Clang crashed");
                break;
    
            case CXError_InvalidArguments:
                spdlog::error("Clang Invalid arguments");
                break;
    
            case CXError_ASTReadError:
                spdlog::error("Clang AST read error");
                break;
            }
        }
        
        std::filesystem::remove(AmalgamationPath.c_str());
        clang_disposeIndex(ClangIndex);
        return Result == CXError_Success;
    }
}
