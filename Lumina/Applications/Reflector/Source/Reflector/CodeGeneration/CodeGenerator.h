#pragma once
#include "Reflector/ReflectionCore/ReflectionDatabase.h"

namespace Lumina::Reflection
{
    class FReflectedWorkspace;

    class FCodeGenerator
    {
    public:

        FCodeGenerator(FReflectedWorkspace* InWorkspace, const FReflectionDatabase& Database);
        
        void GenerateCodeForProject(const FReflectedProject* Project);

        void GenerateReflectionCodeForHeader(const FReflectedHeader& Header);
        void GenerateReflectionCodeForSource(const FReflectedHeader& Header);
    
    private:

        void GenerateCodeHeader(eastl::string& Stream, const FReflectedHeader& Header);
        void GenerateCodeSource(eastl::string& Stream, const FReflectedHeader& Header);


    private:

        
        const FReflectedProject*                CurrentProject = nullptr;
        FReflectedWorkspace*                    Workspace;
        const FReflectionDatabase*              ReflectionDatabase;
        
        
    };
}
