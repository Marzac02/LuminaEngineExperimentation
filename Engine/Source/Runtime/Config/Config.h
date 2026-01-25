#pragma once
#include <nlohmann/json_fwd.hpp>

#include "Core/Object/Object.h"
#include "Core/Object/ObjectMacros.h"
#include "Config.generated.h"

namespace Lumina
{
    
    
    class CConfig;
    
    REFLECT()
    class RUNTIME_API CConfigRegistry : public CObject
    {
        GENERATED_BODY()
    public:
        
        static CConfigRegistry& Get();
        
        void Initialize();
        
        void RegisterConfig(CConfig* Config);
        void OnDestroy() override;
        
        void LoadEngineConfig();
        void LoadProjectConfig();
        void SaveConfigs();
        
        FFixedString GetConfigFilePath(const CConfig* Config) const;
        
    private:
        
        FFixedString ResolvePath(FStringView PathTemplate, FStringView ConfigType);
        
        void LoadConfig(CConfig* Config);
        void SaveConfig(CConfig* Config, FStringView Path);
        
        template<typename T>
        void LoadConfig()
        {
            LoadConfig(GetMutableDefault<T>());
        }
        
        
    private:
        
        TVector<CConfig*>   Configs;
    };
    
    
    REFLECT()
    class RUNTIME_API CConfig : public CObject
    {
        GENERATED_BODY()
    public:
        
        void PostCreateCDO() override;
        
        virtual void SaveConfig(nlohmann::json& Json) LUMINA_PURE_VIRTUAL()
        virtual void LoadConfig(nlohmann::json& Json) LUMINA_PURE_VIRTUAL()
        
        FString GetConfigName() const;
        
    };
}
