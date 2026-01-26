#include "PCH.h"
#include "CommandLine.h"

namespace Lumina
{
    
    RUNTIME_API FCommandLine* CommandLine = nullptr;
    
    namespace Detail
    {
        static FFixedString Normalize(FStringView Raw)
        {
            FFixedString String(Raw.begin(), Raw.end());
            String.make_lower();
            return Move(String);
        }
    }
    
    FCommandLine::FCommandLine(int argc, char* argv[])
    {
        CommandLine = this;
        Parse(argc, argv);
    }

    void FCommandLine::Parse(int argc, char* argv[])
    {
        for (int i = 1; i < argc; ++i)
        {
            FStringView Arg(argv[i], strlen(argv[i]));
    
            if (Arg.starts_with("--"))
            {
                FFixedString Key = Detail::Normalize(Arg.substr(2));
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
    
                Args[Key] = Detail::Normalize(Value);
            }
            else if (Arg.starts_with('-') && Arg.size() > 1)
            {
                for (size_t j = 1; j < Arg.size(); ++j)
                {
                    FFixedString Key(1, Arg[j]);
                    Args[Detail::Normalize(Key)] = "true";
                }
            }
            else
            {
                PositionalArgs.emplace_back(FFixedString(Arg.begin(), Arg.end()));
            }
        }
    }

    bool FCommandLine::Has(const FString& name) const
    {
        return Args.find(Detail::Normalize(name)) != Args.end();
    }

    TOptional<FFixedString> FCommandLine::Get(const FString& Name) const
    {
        auto it = Args.find(Detail::Normalize(Name));
        return it != Args.end() ? TOptional(it->second) : eastl::nullopt;
    }

    TOptional<int> FCommandLine::GetInt(const FString& name) const
    {
        auto it = Args.find(Detail::Normalize(name));
        return it != Args.end() ? TOptional(std::stoi(it->second.c_str())) : eastl::nullopt;
    }

    TOptional<bool> FCommandLine::GetBool(const FString& name) const
    {
        auto it = Args.find(Detail::Normalize(name));
        if (it == Args.end())
        {
            return eastl::nullopt;
        }
    
        const FFixedString& Val = it->second;
        return Val.empty() || Val == "1" || Val == "true" || Val == "yes";
    }

    const TVector<FFixedString>& FCommandLine::GetPositionalArgs() const
    {
        return PositionalArgs;
    }

    const THashMap<FName, FFixedString>& FCommandLine::GetAll() const
    {
        return Args;
    }
}
