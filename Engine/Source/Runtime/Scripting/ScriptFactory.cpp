#include "pch.h"
#include "ScriptFactory.h"

namespace Lumina
{
    static CScriptFactoryRegistry* RegistrySingleton = nullptr;
    
    CScriptFactoryRegistry& CScriptFactoryRegistry::Get()
    {
        static std::once_flag Flag;
        std::call_once(Flag, []()
        {
            RegistrySingleton = NewObject<CScriptFactoryRegistry>();
        });

        return *RegistrySingleton;
    }

    void CScriptFactoryRegistry::RegisterFactory(CScriptFactory* Factory)
    {
        Factories.push_back(Factory);
    }

    void CScriptFactory::PostCreateCDO()
    {
        if (GetClass() != StaticClass())
        {
            CScriptFactoryRegistry::Get().RegisterFactory(this);
        }
    }
}
