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
        DEBUG_ASSERT(Config);

        Configs.emplace_back(Config);
        LoadConfig(Config);
    }

    void CConfigRegistry::OnDestroy()
    {
    }

    void CConfigRegistry::LoadEngineConfig()
    {
        FFixedString ConfigPath = "/Engine/Config";
        
        FFixedString BasePath = ConfigPath + "/Base.json";
        if (!FileSystem::Exists(BasePath))
        {
            SaveConfig(GetMutableDefault<CConfig_Base>(), BasePath);
        }
    }

    void CConfigRegistry::LoadProjectConfig()
    {
        constexpr FStringView ConfigTypes[] = 
        {
            "Game",
            "Engine",
            "Input",
            "Editor",
            "Graphics",
        };
        
        for (const FStringView& ConfigType : ConfigTypes)
        {
            FFixedString ConfigPath = ResolvePath("{PROJECT}/Config/Default{TYPE}.json", ConfigType);
        
            if (!FileSystem::Exists(ConfigPath))
            {
                if (ConfigType == "Game")
                {
                    //SaveConfig(GameConfig, ConfigPath);
                }
            }
            else
            {
                if (ConfigType == "Game")
                {
                    
                }
            }
        }
    }

    void CConfigRegistry::SaveConfigs()
    {
        
    }

    FFixedString CConfigRegistry::GetConfigFilePath(const CConfig* Config) const
    {
        DEBUG_ASSERT(Config);

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

    FFixedString CConfigRegistry::ResolvePath(FStringView PathTemplate, FStringView ConfigType)
    {
        FFixedString Result(PathTemplate.begin(), PathTemplate.end());
    
        size_t EnginePos = Result.find("{ENGINE}");
        if (EnginePos != FFixedString::npos)
        {
            Result.replace(EnginePos, 8, "/Engine");
        }
    
        size_t ProjectPos = Result.find("{PROJECT}");
        if (ProjectPos != FFixedString::npos)
        {
            Result.replace(ProjectPos, 9, "/Game");
        }
    
        if (!ConfigType.empty())
        {
            size_t TypePos = Result.find("{TYPE}");
            if (TypePos != FFixedString::npos)
            {
                Result.replace(TypePos, 6, FFixedString(ConfigType.begin(), ConfigType.end()));
            }
        }
    
        return Result;
    }

    void CConfigRegistry::LoadConfig(CConfig* Config)
    {
        DEBUG_ASSERT(Config);
        
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
        DEBUG_ASSERT(Config);

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
