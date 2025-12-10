#pragma once
#include "ProjectSolution.h"
#include "ReflectionCore/ReflectedProject.h"

namespace Lumina::Reflection
{
    class FClangParser;

    extern uint64_t GTranslationUnitsVisited;
    extern uint64_t GTranslationUnitsParsed;

    class FTypeReflector
    {
    public:

        FTypeReflector(const eastl::string& ReflectionPath);

        /** Gathers all reflectable projects within a solution. */
        bool ParseSolution();

        /** Deletes all previously generated files */
        bool Clean();

        /** Builds the reflection database */
        bool Build(FClangParser& Parser);

        /** Creates and generates reflection code files */
        bool Generate(FClangParser& Parser);

        bool IsAnyProjectDirty() const;

    private:

        bool WriteGeneratedFiles(const FClangParser& Parser);
        
        FProjectSolution Solution;
    };
}
