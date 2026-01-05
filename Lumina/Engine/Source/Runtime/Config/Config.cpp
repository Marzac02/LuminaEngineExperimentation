#include "pch.h"
#include "Config.h"

#include <nlohmann/json.hpp>

#include "Config_Base.h"
#include "Core/Object/Class.h"
#include "FileSystem/FileSystem.h"
#include "Paths/Paths.h"

namespace Lumina
{
    static CConfigRegistry* Singleton = nullptr;

    CConfigRegistry& CConfigRegistry::Get()
    {
        static std::once_flag Flag;
        std::call_once(Flag, []()
        {
            Singleton = NewObject<CConfigRegistry>();
        });

        return *Singleton;
    }

    void CConfigRegistry::Initialize()
    {
        
    }

    void CConfigRegistry::RegisterConfig(CConfig* Config)
    {
        LUM_ASSERT(Config)

        Configs.emplace_back(Config);
        LoadConfig(Config);
    }

    void CConfigRegistry::OnDestroy()
    {
    }

    void CConfigRegistry::LoadConfigInDirectory(FStringView Directory)
    {
        FFixedString Path;
        Path.append(Directory.begin(), Directory.end()).append("/Base.json");
        if (!FileSystem::Exists(Path))
        {
            SaveConfig(GetMutableDefault<CConfig_Base>(), Path);
        }
    }

    void CConfigRegistry::SaveConfigs()
    {
        
    }

    FFixedString CConfigRegistry::GetConfigFilePath(CConfig* Config) const
    {
        LUM_ASSERT(Config)

        FString ConfigName = Config->GetConfigName();
        if (ConfigName.empty())
        {
            return {};
        }
        
        return FFixedString()
                    .assign("Config/")
                    .append_convert(ConfigName)
                    .append(".json");
    }

    void CConfigRegistry::LoadConfig(CConfig* Config)
    {
        LUM_ASSERT(Config)
        
        FString ConfigName = Config->GetConfigName();
        FFixedString DefaultPath = GetConfigFilePath(Config);
        if (DefaultPath.empty())
        {
            return;
        }
        
        FString JsonContent;
        if (FileSystem::ReadFile(JsonContent, DefaultPath))
        {
            try
            {
                nlohmann::json Json = nlohmann::json::parse(JsonContent.c_str());
                Config->LoadConfig(Json);
            }
            catch (const std::exception& e)
            {
                LOG_ERROR("Failed to load config: {0}", e.what());
            }
        }
    }

    void CConfigRegistry::SaveConfig(CConfig* Config, FStringView Path)
    {
        LUM_ASSERT(Config)

        FFixedString Header = "// Lumina Engine Configuration File\n";
        
        nlohmann::json Json;
        
        Config->SaveConfig(Json);
        
        FFixedString JsonString;
        JsonString.assign_convert(Json.dump(4));
        
        FileSystem::WriteFile(Path, Header + JsonString);
    }

    void CConfig::PostCreateCDO()
    {
        CConfigRegistry::Get().RegisterConfig(this);
    }

    FString CConfig::GetConfigName() const
    {
        const CClass* Class = GetClass();
        if (!Class->HasMeta("Config"))
        {
            return {};
        }
        
        return Class->GetMeta("Config").ToString();
    }
}
