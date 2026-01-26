#include "pch.h"
#include "Config.h"
#include <nlohmann/json.hpp>
#include "FileSystem/FileSystem.h"

namespace FS = Lumina::FileSystem;
using Json = nlohmann::json;

namespace Lumina
{
    RUNTIME_API FConfig* GConfig;
    
    void FConfig::LoadPath(FStringView ConfigPath)
    {
        FS::DirectoryIterator(ConfigPath, [&](const FS::FFileInfo& Info)
        {
            if (Info.GetExt() != ".json")
            {
                return;
            }
            
            FString Result;
            FS::ReadFile(Result, Info.VirtualPath);
            
            Json J = Json::parse(Result.c_str());
            MergeJson(RootConfig, J);
        });
    }

    bool FConfig::FCategory::Has(FStringView Key) const
    {
        return Node.contains(Key.data());
    }

    FConfig::FCategory FConfig::FCategory::GetCategory(FStringView CategoryName) const
    {
        if (!Node.contains(CategoryName.data()))
        {
            Node[CategoryName.data()] = nlohmann::json::object();
        }
        return FCategory(Node[CategoryName.data()]);
    }

    bool FConfig::Set(const FString& Path, const nlohmann::json& Value)
    {
        if (Path.empty())
        {
            return false;
        }
    
        TVector<FString> PathParts;
        std::string PathStr = Path.c_str();
        std::string Delimiter = ".";
        size_t Pos = 0;
    
        while ((Pos = PathStr.find(Delimiter)) != std::string::npos)
        {
            PathParts.emplace_back(PathStr.substr(0, Pos).c_str());
            PathStr.erase(0, Pos + Delimiter.length());
        }
        PathParts.emplace_back(PathStr.c_str());
    
        if (PathParts.empty())
        {
            return false;
        }
    
        nlohmann::json* Current = &RootConfig;
    
        for (size_t i = 0; i < PathParts.size() - 1; ++i)
        {
            const FString& Part = PathParts[i];
        
            if (!Current->contains(Part.c_str()) || !(*Current)[Part.c_str()].is_object())
            {
                (*Current)[Part.c_str()] = nlohmann::json::object();
            }
        
            Current = &(*Current)[Part.c_str()];
        }
    
        const FString& FinalKey = PathParts.back();
        (*Current)[FinalKey.c_str()] = Value;
    
        return true;
    }

    FConfig::FCategory FConfig::GetCategory(FStringView CategoryName)
    {
        if (!RootConfig.contains(CategoryName.data()))
        {
            RootConfig[CategoryName.data()] = nlohmann::json::object();
        }
        return FCategory(RootConfig[CategoryName.data()]);
    }

    void FConfig::MergeJson(nlohmann::json& Target, const nlohmann::json& Source)
    {
        for (auto it = Source.begin(); it != Source.end(); ++it)
        {
            if (it->is_object() && Target.contains(it.key()) && Target[it.key()].is_object())
            {
                MergeJson(Target[it.key()], *it);
            }
            else
            {
                Target[it.key()] = *it;
            }
        }
    }

    nlohmann::json* FConfig::NavigateToNode(FStringView Path)
    {
        nlohmann::json* Current = &RootConfig;
    
        size_t Start = 0;
        size_t Pos = 0;
    
        while (Pos < Path.size())
        {
            if (Path[Pos] == '.')
            {
                FStringView Segment = Path.substr(Start, Pos - Start);
                if (!Current->contains(Segment.data()))
                {
                    return nullptr;
                }
                Current = &(*Current)[Segment.data()];
                Start = Pos + 1;
            }
            Pos++;
        }
    
        if (Start < Path.size())
        {
            FStringView Segment = Path.substr(Start);
            if (!Current->contains(Segment.data()))
            {
                return nullptr;
            }
            Current = &(*Current)[Segment.data()];
        }
    
        return Current;
    }

    const nlohmann::json* FConfig::NavigateToNode(FStringView Path) const
    {
        return const_cast<FConfig*>(this)->NavigateToNode(Path);
    }
}
