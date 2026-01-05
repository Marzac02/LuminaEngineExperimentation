#pragma once
#include "Config.h"
#include "Config_Base.generated.h"

namespace Lumina
{
    REFLECT()
    class CConfig_Base : public CConfig
    {
        GENERATED_BODY()
    public:
        
        void SaveConfig(nlohmann::json& Json) override;
        void LoadConfig(nlohmann::json& Json) override;
    
    };
}
