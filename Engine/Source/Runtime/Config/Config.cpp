#include "pch.h"
#include "Config.h"
#include <nlohmann/json.hpp>
#include "FileSystem/FileSystem.h"

using Json = nlohmann::json;

namespace Lumina
{
    RUNTIME_API FConfig* GConfig;
    
    void FConfig::LoadPath(FStringView ConfigPath)
    {
        VFS::DirectoryIterator(ConfigPath, [&](const VFS::FFileInfo& Info)
        {
            if (Info.GetExt() != ".json")
            {
                return;
            }
            
            FString Result;
            VFS::ReadFile(Result, Info.VirtualPath);
            
            Json J = Json::parse(Result.c_str());
            
            FileConfigs[Info.VirtualPath.c_str()] = J;
            
            TFunction<void(const FString&, const Json&)> TrackPaths;
            TrackPaths = [&](const FString& Prefix, const Json& Obj)
            {
                for (auto It = Obj.begin(); It != Obj.end(); ++It)
                {
                    FString Key = It.key().c_str();
                    FString FullPath = Prefix.empty() ? Key : std::format("{}.{}", Prefix, Key).c_str();
                    
                    PathToFile[FullPath] = Info.VirtualPath;
                    
                    if (It->is_object())
                    {
                        TrackPaths(FullPath, *It);
                    }
                }
            };
            
            TrackPaths("", J);
            
            MergeJson(RootConfig, J);
        });
    }

    const nlohmann::json* FConfig::GetJson(FStringView Key)
    {
        return NavigateToNode(Key);
    }

    bool FConfig::Set(const FString& Path, const nlohmann::json& Value)
    {
        if (Path.empty())
        {
            return false;
        }
    
        FString SourceFile = FindSourceFile(Path);
        
        if (SourceFile.empty())
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
        (*Current)[PathParts.back().c_str()] = Value;
        
        Current = &FileConfigs[SourceFile];
        for (size_t i = 0; i < PathParts.size() - 1; ++i)
        {
            const FString& Part = PathParts[i];
            if (!Current->contains(Part.c_str()) || !(*Current)[Part.c_str()].is_object())
            {
                (*Current)[Part.c_str()] = nlohmann::json::object();
            }
            Current = &(*Current)[Part.c_str()];
        }
        (*Current)[PathParts.back().c_str()] = Value;
        
        FString JsonString = FileConfigs[SourceFile.c_str()].dump(4).c_str();
        VFS::WriteFile(SourceFile, JsonString);
    
        return true;
    }

    FString FConfig::FindSourceFile(const FString& Path) const
    {
        auto It = PathToFile.find(Path);
        if (It != PathToFile.end())
        {
            return It->second;
        }
        
        FString CurrentPath = Path;
        size_t LastDot = CurrentPath.find_last_of('.');
        
        while (LastDot != FString::npos)
        {
            CurrentPath = CurrentPath.substr(0, LastDot);
            It = PathToFile.find(CurrentPath);
            if (It != PathToFile.end())
            {
                return It->second;
            }
            LastDot = CurrentPath.find_last_of('.');
        }
        
        return "";
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
