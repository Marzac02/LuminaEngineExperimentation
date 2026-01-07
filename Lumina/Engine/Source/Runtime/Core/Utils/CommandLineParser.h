#pragma once

#include "Containers/Array.h"
#include "Containers/Name.h"
#include "Core/Templates/LuminaTemplate.h"
#include "Core/Templates/Optional.h"

namespace Lumina
{
    class FCommandLineParser
    {
    public:
    
        FCommandLineParser() = default;
    
        FCommandLineParser(int argc, char* argv[])
        {
            Parse(argc, argv);
        }
    
        void Parse(int argc, char* argv[])
        {
            for (int i = 1; i < argc; ++i)
            {
                FStringView Arg(argv[i], strlen(argv[i]));
    
                if (Arg.starts_with("--"))
                {
                    FFixedString Key = Normalize(Arg.substr(2));
                    FFixedString Value;
    
                    size_t Equals = Key.find('=');
                    if (Equals != FString::npos)
                    {
                        Value   = Key.substr(Equals + 1, 0);
                        Key     = Key.substr(0, Equals);
                    }
                    else if (i + 1 < argc)
                    {
                        FStringView NextArg(argv[i + 1]);
                        if (NextArg.starts_with("--") == false)
                        {
                            Value = argv[++i];
                        }
                    }
    
                    Args[Key] = Normalize(Value);
                }
                else if (Arg.starts_with('-') && Arg.size() > 1)
                {
                    for (size_t j = 1; j < Arg.size(); ++j)
                    {
                        FFixedString Key(1, Arg[j]);
                        Args[Normalize(Key)] = "true";
                    }
                }
                else
                {
                    PositionalArgs.emplace_back(FFixedString(Arg.begin(), Arg.end()));
                }
            }
        }
    
        NODISCARD bool Has(const FString& name) const
        {
            return Args.find(Normalize(name)) != Args.end();
        }
    
        NODISCARD TOptional<FFixedString> Get(const FString& Name) const
        {
            auto it = Args.find(Normalize(Name));
            return it != Args.end() ? TOptional(it->second) : eastl::nullopt;
        }
    
        NODISCARD TOptional<int> GetInt(const FString& name) const
        {
            auto it = Args.find(Normalize(name));
            return it != Args.end() ? TOptional(std::stoi(it->second.c_str())) : eastl::nullopt;
        }
    
        NODISCARD TOptional<bool> GetBool(const FString& name) const
        {
            auto it = Args.find(Normalize(name));
            if (it == Args.end())
            {
                return eastl::nullopt;
            }
    
            const FFixedString& Val = it->second;
            return Val.empty() || Val == "1" || Val == "true" || Val == "yes";
        }
    
        const TVector<FFixedString>& GetPositionalArgs() const
        {
            return PositionalArgs;
        }

        const THashMap<FName, FFixedString>& GetAll() const
        {
            return Args;
        }
    
    private:
    
        static FFixedString Normalize(FStringView Raw)
        {
            FFixedString String(Raw.begin(), Raw.end());
            String.make_lower();
            return Move(String);
        }
    
    private:
        THashMap<FName, FFixedString>   Args;
        TVector<FFixedString>           PositionalArgs;
    };

}
