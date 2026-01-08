#pragma once
#include "Core/Object/Object.h"
#include "ScriptTypes.h"
#include "Core/Utils/Expected.h"
#include "Memory/SmartPtr.h"
#include "ScriptFactory.generated.h"

namespace Lumina
{
    class CScriptFactory;

    
    REFLECT()
    class LUMINA_API CScriptFactoryRegistry : public CObject
    {
        GENERATED_BODY()
    public:

        static CScriptFactoryRegistry& Get();

        void RegisterFactory(CScriptFactory* Factory);

        FORCEINLINE void GetFactories(TVector<CScriptFactory*>& OutFactories) const { OutFactories = Factories; }

    private:

        TVector<CScriptFactory*> Factories;
    };

    REFLECT()
    class LUMINA_API CScriptFactory : public CObject
    {
        GENERATED_BODY()
    public:
        
        void PostCreateCDO() override;
        virtual entt::entity ProcessScript(FName Name, const sol::table& ScriptTable, entt::registry& ScriptRegistry) const { return entt::null; }
    };
}
