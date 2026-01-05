#pragma once
#include <nlohmann/json_fwd.hpp>

#include "Core/Object/Object.h"
#include "Core/Object/ObjectMacros.h"
#include "Config.generated.h"

namespace Lumina
{
    class CConfig;
    
    REFLECT()
    class LUMINA_API CConfigRegistry : public CObject
    {
        GENERATED_BODY()
    public:
        
        static CConfigRegistry& Get();
        
        void Initialize(FStringView Project);
        
        void RegisterConfig(CConfig* Config);
        void OnDestroy() override;
        
        void LoadConfigs();
        void SaveConfigs();
        
        FFixedString GetConfigFilePath(CConfig* Config) const;
        
    private:
        
        void LoadConfig(CConfig* Config);
        void SaveConfig(CConfig* Config);
        
        
    private:
        
        TVector<CConfig*>   Configs;
        FFixedString        ProjectPath;
    };
    
    
    REFLECT()
    class LUMINA_API CConfig : public CObject
    {
        GENERATED_BODY()
    public:
        
        void PostCreateCDO() override;
        
        virtual void SaveConfig(nlohmann::json& Json) LUMINA_PURE_VIRTUAL()
        virtual void LoadConfig(nlohmann::json& Json) LUMINA_PURE_VIRTUAL()
        
        FString GetConfigName() const;
        
    };
}
