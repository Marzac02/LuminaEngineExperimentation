#pragma once
#include "Reflector/ReflectionCore/ReflectionDatabase.h"

namespace Lumina::Reflection
{
    class FCodeGenerator
    {
    public:

        FCodeGenerator(const FProjectSolution& SlnPath, const FReflectionDatabase& Database);


        void GenerateCodeForSolution();
        void GenerateCodeForProject(const eastl::shared_ptr<FReflectedProject>& Project);

        void GenerateReflectionCodeForHeader(const FReflectedHeader& Header);
        void GenerateReflectionCodeForSource(const FReflectedHeader& Header);

        void SetProject(const eastl::shared_ptr<FReflectedProject>& Project) { CurrentProject = Project; }

    private:

        void GenerateCodeHeader(eastl::string& Stream, const FReflectedHeader& Header);
        void GenerateCodeSource(eastl::string& Stream, const FReflectedHeader& Header);


    private:

        
        eastl::shared_ptr<FReflectedProject>    CurrentProject;
        FProjectSolution                        Solution;
        const FReflectionDatabase*              ReflectionDatabase;
        
        
    };
}
