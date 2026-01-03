#pragma once
#include "EASTL/hash_map.h"
#include "EASTL/queue.h"
#include "Engine/Source/Runtime/Platform/GenericPlatform.h"
#include "Reflector/TypeReflector.h"
#include "Reflector/ReflectionCore/ReflectionDatabase.h"
#include "Reflector/ReflectionCore/ReflectionMacro.h"

namespace Lumina::Reflection
{
    class FClangParserContext
    {
    public:

        FClangParserContext()
            : ParentReflectedType(nullptr)
            , LastReflectedType(nullptr)
            , Solution("")
        {}
        
        ~FClangParserContext() = default;
        FClangParserContext(const FClangParserContext&) = delete;
        FClangParserContext(FClangParserContext&&) = delete;
        FClangParserContext& operator = (const FClangParserContext&) = delete;
        FClangParserContext& operator = (FClangParserContext&&) = delete;
        
        void AddReflectedMacro(FReflectionMacro&& Macro);
        void AddGeneratedBodyMacro(FReflectionMacro&& Macro);
        
        bool TryFindMacroForCursor(const eastl::string& HeaderID, const CXCursor& Cursor, FReflectionMacro& Macro);

        bool TryFindGeneratedBodyMacro(const eastl::string& HeaderID, const CXCursor& Cursor, FReflectionMacro& Macro);

        void LogError(char const* ErrorFormat, ...) const;
        void LogWarning(char const* ErrorFormat, ...) const;
        void FlushLogs();

        bool HasError() const { return !ErrorMessage.empty(); }


        void PushNamespace(const eastl::string& Namespace);
        void PopNamespace();

        template<typename T>
        T* GetParentReflectedType();

        
        FReflectedType*                                             ParentReflectedType;
        FReflectedType*                                             LastReflectedType;
                                                                    
        FReflectionDatabase                                         ReflectionDatabase;
        
        mutable eastl::string                                       ErrorMessage;
        mutable eastl::string                                       WarningMessage;
        
        FProjectSolution                                            Solution;
        eastl::shared_ptr<FReflectedProject>                        Project = nullptr;
        eastl::shared_ptr<FReflectedHeader>                         ReflectedHeader = nullptr;
        
        eastl::hash_map<uint64, eastl::vector<FReflectionMacro>>    ReflectionMacros;
        eastl::hash_map<uint64, eastl::queue<FReflectionMacro>>     GeneratedBodyMacros;
        
        eastl::vector<eastl::string>                                NamespaceStack;
        eastl::string                                               CurrentNamespace;
                                                                    
        uint32                                                      NumHeadersReflected = 0;

        bool                                                        bInitialPass = true;
        
    };

    template <typename T>
    T* FClangParserContext::GetParentReflectedType()
    {
        return static_cast<T*>(ParentReflectedType);
    }
}
