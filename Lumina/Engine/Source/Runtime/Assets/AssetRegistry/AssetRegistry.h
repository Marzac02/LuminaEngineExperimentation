#pragma once

#include "AssetData.h"
#include "Core/DisableAllWarnings.h"
#include "Subsystems/Subsystem.h"
#include "Core/Delegates/Delegate.h"
#include "Core/Serialization/MemoryArchiver.h"
#include "Core/Templates/LuminaTemplate.h"
#include "Core/Threading/Thread.h"
#include "Memory/SmartPtr.h"



DECLARE_MULTICAST_DELEGATE(FAssetRegistryUpdatedDelegate);

namespace Lumina
{
	class CPackage;
	struct FAssetData;
	class CClass;

	struct FAssetDataPtrHash
	{
		size_t operator() (const TSharedPtr<FAssetData>& Asset) const noexcept
		{
			return Hash::GetHash(Asset->AssetGUID);
		}
	};

	struct FAssetDataPtrEqual
	{
		bool operator()(const TSharedPtr<FAssetData>& A, const TSharedPtr<FAssetData>& B) const noexcept
		{
			return A->AssetGUID == B->AssetGUID;
		}
	};

    struct FGuidHash
    {
        size_t operator()(const FGuid& GUID) const noexcept
        {
            return Hash::GetHash(GUID);
        }
    };

    struct FAssetDataGuidEqual
    {
        bool operator()(const TSharedPtr<FAssetData>& Asset, const FGuid& GUID) const noexcept
        {
            return Asset->AssetGUID == GUID;
        }
    };

	using FAssetDataMap = THashSet<TSharedPtr<FAssetData>, FAssetDataPtrHash, FAssetDataPtrEqual>;
	
	
	class LUMINA_API FAssetRegistry final
	{
	public:

		static FAssetRegistry& Get();
		
		void ProjectLoaded();

		void RunInitialDiscovery();
		void OnInitialDiscoveryCompleted();

		void AssetCreated(CObject* Asset);
		void AssetDeleted(const FGuid& GUID);
		void AssetRenamed(const FString& OldPath, const FString& NewPath);
		void AssetSaved(CObject* Asset);

		FAssetRegistryUpdatedDelegate& GetOnAssetRegistryUpdated() { return OnAssetRegistryUpdated; }
		
		const FAssetDataMap& GetAssets() const { return Assets; }

	private:

		void ProcessPackagePath(FStringView Path);
		
		void ClearAssets();

		void BroadcastRegistryUpdate();


		FAssetRegistryUpdatedDelegate	OnAssetRegistryUpdated;
		
		FMutex							AssetsMutex;

		/** Global hash of all registered assets */
		FAssetDataMap 					Assets;
	};

    
    class IAssetPredicate;

    template<typename T>
    struct is_predicate : eastl::false_type {};

    template<typename Ret, typename... Args>
    struct is_predicate<eastl::function<Ret(Args...)>> : eastl::true_type {};

    template<typename T>
    constexpr bool is_predicate_v = is_predicate<T>::value;

    // Compile-time predicate composition detector
    template<typename T>
    struct is_combinable_predicate : eastl::bool_constant<eastl::is_base_of_v<IAssetPredicate, T>> {};

    // ============================================================================
    // Predicate Base Classes
    // ============================================================================

    class LUMINA_API IAssetPredicate
    {
    public:
        virtual ~IAssetPredicate() = default;
        virtual bool Evaluate(const FAssetData& Asset) const = 0;
        virtual FString ToString() const = 0;
        virtual TSharedPtr<IAssetPredicate> Clone() const = 0;
    };

    // ============================================================================
    // Concrete Predicates
    // ============================================================================

    class LUMINA_API FPathPredicate : public IAssetPredicate
    {
        FString Path;
        bool bExactMatch;
        bool bRecursive;
    
    public:
        explicit FPathPredicate(FString InPath, bool bExact = false, bool bRec = true)
            : Path(Move(InPath)), bExactMatch(bExact), bRecursive(bRec) {}
    
        bool Evaluate(const FAssetData& Asset) const override
        {
            const FString& AssetPath = Asset.FilePath;
        
            if (bExactMatch)
            {
                return AssetPath == Path;
            }

            if (bRecursive)
            {
                return AssetPath.find(Path) != FString::npos;
            }

            // Non-recursive: check if in direct child of path
            if (AssetPath.find(Path) == 0)
            {
                size_t PathLen = Path.length();
                size_t NextSlash = AssetPath.find('/', PathLen + 1);
                return NextSlash == FString::npos;
            }
            return false;
        }
    
        FString ToString() const override
        {
            return FString().sprintf("Path%s(%s)%s", 
                bExactMatch ? "Exact" : "Contains",
                Path.c_str(),
                bRecursive ? "" : "[NonRecursive]");
        }
    
        TSharedPtr<IAssetPredicate> Clone() const override
        {
            return MakeSharedPtr<FPathPredicate>(Path, bExactMatch, bRecursive);
        }
    };

    class LUMINA_API FGuidPredicate : public IAssetPredicate
    {
        FGuid Guid;
    
    public:
        explicit FGuidPredicate(const FGuid& InGuid) : Guid(InGuid) {}
    
        bool Evaluate(const FAssetData& Asset) const override
        {
            return Asset.AssetGUID == Guid;
        }
    
        FString ToString() const override
        {
            return FString().sprintf("Guid(%s)", Guid.ToString().c_str());
        }
    
        TSharedPtr<IAssetPredicate> Clone() const override
        {
            return MakeSharedPtr<FGuidPredicate>(Guid);
        }
    };

    class LUMINA_API FClassPredicate : public IAssetPredicate
    {
        FName ClassName;
        bool bIncludeDerived;
    
    public:
        explicit FClassPredicate(FName InClassName, bool bDerived = true)
            : ClassName(InClassName), bIncludeDerived(bDerived) {}
    
        bool Evaluate(const FAssetData& Asset) const override
        {
            if (!bIncludeDerived)
            {
                return Asset.AssetClass == ClassName;
            }

            // Check class hierarchy - would need reflection system integration
            // For now, simplified direct match
            return Asset.AssetClass == ClassName;
        }
    
        FString ToString() const override
        {
            return FString().sprintf("Class(%s%s)", 
                ClassName.ToString().c_str(),
                bIncludeDerived ? "+Derived" : "");
        }
    
        TSharedPtr<IAssetPredicate> Clone() const override
        {
            return MakeSharedPtr<FClassPredicate>(ClassName, bIncludeDerived);
        }
    };

    class LUMINA_API FNamePredicate : public IAssetPredicate
    {
        FString NamePattern;
        bool bCaseSensitive;
    
    public:
        explicit FNamePredicate(FString InPattern, bool bCase = false)
            : NamePattern(Move(InPattern)), bCaseSensitive(bCase) {}
    
        bool Evaluate(const FAssetData& Asset) const override
        {
            FString AssetName = Asset.AssetName.ToString();
        
            if (!bCaseSensitive)
            {
                AssetName.make_lower();
                FString Pattern = NamePattern;
                Pattern.make_lower();
                return AssetName.find(Pattern) != FString::npos;
            }
        
            return AssetName.find(NamePattern) != FString::npos;
        }
    
        FString ToString() const override
        {
            return FString().sprintf("Name(%s)%s", 
                NamePattern.c_str(),
                bCaseSensitive ? "[CaseSensitive]" : "");
        }
    
        TSharedPtr<IAssetPredicate> Clone() const override
        {
            return MakeSharedPtr<FNamePredicate>(NamePattern, bCaseSensitive);
        }
    };

    // Template-based lambda predicate with perfect forwarding
    template<typename Func>
    class TLambdaPredicate : public IAssetPredicate
    {
        Func Lambda;
        FString Description;
    
    public:
        explicit TLambdaPredicate(Func&& InLambda, FString InDesc = "Custom")
            : Lambda(eastl::forward<Func>(InLambda)), Description(Move(InDesc)) {}
    
        bool Evaluate(const FAssetData& Asset) const override
        {
            return Lambda(Asset);
        }
    
        FString ToString() const override
        {
            return FString().sprintf("Lambda(%s)", Description.c_str());
        }
    
        TSharedPtr<IAssetPredicate> Clone() const override
        {
            return MakeSharedPtr<TLambdaPredicate<Func>>(eastl::forward<Func>(const_cast<Func&>(Lambda)), Description);
        }
    };

    // Helper to create lambda predicates with type deduction
    template<typename Func>
    TSharedPtr<IAssetPredicate> MakeLambdaPredicate(Func&& Lambda, FString Desc = "Custom")
    {
        return MakeSharedPtr<TLambdaPredicate<eastl::decay_t<Func>>>(eastl::forward<Func>(Lambda), Move(Desc));
    }

    // ============================================================================
    // Logical Combinators with SFINAE
    // ============================================================================

    class LUMINA_API FAndPredicate : public IAssetPredicate
    {
        TVector<TSharedPtr<IAssetPredicate>> Predicates;
    
    public:
        explicit FAndPredicate(TVector<TSharedPtr<IAssetPredicate>> InPredicates)
            : Predicates(Move(InPredicates)) {}
    
        bool Evaluate(const FAssetData& Asset) const override
        {
            for (const auto& Pred : Predicates)
            {
                if (!Pred || !Pred->Evaluate(Asset))
                {
                    return false;
                }
            }
            return !Predicates.empty();
        }
    
        FString ToString() const override
        {
            FString Result = "(";
            for (size_t i = 0; i < Predicates.size(); ++i)
            {
                if (i > 0)
                {
                    Result += " AND ";
                }
                Result += Predicates[i]->ToString();
            }
            Result += ")";
            return Result;
        }
    
        TSharedPtr<IAssetPredicate> Clone() const override
        {
            TVector<TSharedPtr<IAssetPredicate>> ClonedPreds;
            ClonedPreds.reserve(Predicates.size());
            for (const auto& P : Predicates)
            {
                ClonedPreds.push_back(P->Clone());
            }
            return MakeSharedPtr<FAndPredicate>(Move(ClonedPreds));
        }
    };
    
    class LUMINA_API FOrPredicate : public IAssetPredicate
    {
        TVector<TSharedPtr<IAssetPredicate>> Predicates;
    
    public:
        explicit FOrPredicate(TVector<TSharedPtr<IAssetPredicate>> InPredicates)
            : Predicates(Move(InPredicates)) {}
    
        bool Evaluate(const FAssetData& Asset) const override
        {
            for (const auto& Pred : Predicates)
            {
                if (Pred && Pred->Evaluate(Asset))
                {
                    return true;
                }
            }
            return false;
        }
    
        FString ToString() const override
        {
            FString Result = "(";
            for (size_t i = 0; i < Predicates.size(); ++i)
            {
                if (i > 0)
                {
                    Result += " OR ";
                }
                Result += Predicates[i]->ToString();
            }
            Result += ")";
            return Result;
        }
    
        TSharedPtr<IAssetPredicate> Clone() const override
        {
            TVector<TSharedPtr<IAssetPredicate>> ClonedPreds;
            ClonedPreds.reserve(Predicates.size());
            for (const auto& P : Predicates)
            {
                ClonedPreds.push_back(P->Clone());
            }
            return MakeSharedPtr<FOrPredicate>(Move(ClonedPreds));
        }
    };

    class LUMINA_API FNotPredicate : public IAssetPredicate
    {
        TSharedPtr<IAssetPredicate> Predicate;
    
    public:
        explicit FNotPredicate(TSharedPtr<IAssetPredicate> InPredicate)
            : Predicate(Move(InPredicate)) {}
    
        bool Evaluate(const FAssetData& Asset) const override
        {
            return Predicate ? !Predicate->Evaluate(Asset) : true;
        }
    
        FString ToString() const override
        {
            return FString().sprintf("NOT(%s)", Predicate->ToString().c_str());
        }
    
        TSharedPtr<IAssetPredicate> Clone() const override
        {
            return MakeSharedPtr<FNotPredicate>(Predicate->Clone());
        }
    };
    
    class LUMINA_API FAssetQuery
    {
    public:
        FAssetQuery() = default;
        FAssetQuery(const FAssetQuery&) = default;
        FAssetQuery(FAssetQuery&&) = default;
        FAssetQuery& operator=(const FAssetQuery&) = default;
        FAssetQuery& operator=(FAssetQuery&&) noexcept = default;
        
        FAssetQuery& WithPath(const FString& Path, bool bExact = false, bool bRecursive = true)
        {
            AddPredicate(MakeSharedPtr<FPathPredicate>(Path, bExact, bRecursive));
            return *this;
        }
    
        FAssetQuery& WithGuid(const FGuid& Guid)
        {
            AddPredicate(MakeSharedPtr<FGuidPredicate>(Guid));
            return *this;
        }
    
        FAssetQuery& OfClass(FName ClassName, bool bIncludeDerived = true)
        {
            AddPredicate(MakeSharedPtr<FClassPredicate>(ClassName, bIncludeDerived));
            return *this;
        }
    
        template<typename T>
        FAssetQuery& OfClass(bool bIncludeDerived = true)
        {
            static_assert(eastl::is_base_of_v<CObject, T>, "T must derive from CObject");
            return OfClass(T::StaticClass()->GetFName(), bIncludeDerived);
        }
    
        FAssetQuery& WithName(const FString& Pattern, bool bCaseSensitive = false)
        {
            AddPredicate(MakeSharedPtr<FNamePredicate>(Pattern, bCaseSensitive));
            return *this;
        }
    
        FAssetQuery& WithTag(FName Key, const FString& Value = FString())
        {
            //AddPredicate(MakeSharedPtr<FTagPredicate>(Key, Value));
            return *this;
        }
    
        template<typename Func>
        FAssetQuery& Where(Func&& Lambda, FString Description = "Custom")
        {
            static_assert(eastl::is_invocable_r_v<bool, Func, const FAssetData&>, "Lambda must be callable with signature: bool(const FAssetData&)");
        
            AddPredicate(MakeLambdaPredicate(eastl::forward<Func>(Lambda), 
                Move(Description)));
            return *this;
        }
    
        // ========================================================================
        // Logical Operators
        // ========================================================================
    
        FAssetQuery& And()
        {
            PushLogicalOperator(ELogicalOp::And);
            return *this;
        }
    
        FAssetQuery& Or()
        {
            PushLogicalOperator(ELogicalOp::Or);
            return *this;
        }
    
        FAssetQuery& Not()
        {
            bNegateNext = true;
            return *this;
        }
    
        // ========================================================================
        // Grouping with Stack-Based Management
        // ========================================================================
    
        FAssetQuery& BeginGroup()
        {
            GroupStack.push_back(Move(CurrentPredicates));
            CurrentPredicates.clear();
            LogicalOpStack.push_back(Move(LogicalOps));
            LogicalOps.clear();
            return *this;
        }
    
        FAssetQuery& EndGroup()
        {
            if (!GroupStack.empty())
            {
                auto GroupPredicates = Move(CurrentPredicates);
                CurrentPredicates = Move(GroupStack.back());
                GroupStack.pop_back();
            
                auto GroupOps = Move(LogicalOps);
                LogicalOps = Move(LogicalOpStack.back());
                LogicalOpStack.pop_back();
            
                if (!GroupPredicates.empty())
                {
                    auto GroupPred = CombinePredicatesWithOps(Move(GroupPredicates), Move(GroupOps));
                    AddPredicate(Move(GroupPred));
                }
            }
            return *this;
        }
    
        // ========================================================================
        // Execution Methods
        // ========================================================================
    
        TVector<FAssetData> Execute() const
        {
            FAssetRegistry& Registry = FAssetRegistry::Get();
            TVector<FAssetData> Results;
            auto FinalPredicate = BuildFinalPredicate();
        
            if (!FinalPredicate)
            {
                const auto& Assets = Registry.GetAssets();
                Results.reserve(Assets.size());
                for (const auto& Pair : Assets)
                {
                    Results.push_back(*Pair);
                }
                return Results;
            }
        
            const auto& Assets = Registry.GetAssets();
            for (const auto& Pair : Assets)
            {
                if (FinalPredicate->Evaluate(*Pair))
                {
                    Results.push_back(*Pair);
                }
            }
        
            return Results;
        }
    
        // Single result helpers
        const FAssetData* ExecuteFirst() const
        {
            FAssetRegistry& Registry = FAssetRegistry::Get();

            auto FinalPredicate = BuildFinalPredicate();
            if (!FinalPredicate)
            {
                return nullptr;
            }

            const auto& Assets = Registry.GetAssets();
            for (const auto& Pair : Assets)
            {
                if (FinalPredicate->Evaluate(*Pair))
                {
                    return Pair.get();
                }
            }
            return nullptr;
        }
    
        size_t Count() const
        {
            FAssetRegistry& Registry = FAssetRegistry::Get();

            auto FinalPredicate = BuildFinalPredicate();
            if (!FinalPredicate)
            {
                return Registry.GetAssets().size();
            }

            size_t Count = 0;
            const auto& Assets = Registry.GetAssets();
            for (const auto& Pair : Assets)
            {
                if (FinalPredicate->Evaluate(*Pair))
                {
                    ++Count;
                }
            }
            return Count;
        }
    
        bool Any() const
        {
            return ExecuteFirst() != nullptr;
        }
    
        bool All(const FAssetRegistry& Registry) const
        {
            auto FinalPredicate = BuildFinalPredicate();
            if (!FinalPredicate)
            {
                return true;
            }

            const auto& Assets = Registry.GetAssets();
            for (const auto& Pair : Assets)
            {
                if (!FinalPredicate->Evaluate(*Pair))
                {
                    return false;
                }
            }
            return true;
        }
    
        // ========================================================================
        // Query Inspection
        // ========================================================================
    
        FString ToString() const
        {
            auto FinalPredicate = BuildFinalPredicate();
            return FinalPredicate ? FinalPredicate->ToString() : "EmptyQuery";
        }
    
        FAssetQuery Clone() const
        {
            FAssetQuery Copy;
            Copy.CurrentPredicates.reserve(CurrentPredicates.size());
            for (const auto& P : CurrentPredicates)
            {
                Copy.CurrentPredicates.push_back(P->Clone());
            }
            Copy.LogicalOps = LogicalOps;
            Copy.bNegateNext = bNegateNext;
            return Copy;
        }
    
    private:
        enum class ELogicalOp : uint8 { And, Or };
    
        TVector<TSharedPtr<IAssetPredicate>> CurrentPredicates;
        TVector<ELogicalOp> LogicalOps;
        TVector<TVector<TSharedPtr<IAssetPredicate>>> GroupStack;
        TVector<TVector<ELogicalOp>> LogicalOpStack;
        bool bNegateNext = false;
    
        void AddPredicate(TSharedPtr<IAssetPredicate> Predicate)
        {
            if (bNegateNext)
            {
                Predicate = MakeSharedPtr<FNotPredicate>(Move(Predicate));
                bNegateNext = false;
            }
            CurrentPredicates.push_back(Move(Predicate));
        }
    
        void PushLogicalOperator(ELogicalOp Op)
        {
            LogicalOps.push_back(Op);
        }
    
        TSharedPtr<IAssetPredicate> CombinePredicatesWithOps(TVector<TSharedPtr<IAssetPredicate>> Predicates, TVector<ELogicalOp> Ops) const
        {
            if (Predicates.empty())
            {
                return nullptr;
            }
            if (Predicates.size() == 1)
            {
                return Predicates[0];
            }

            // Build expression tree respecting operator precedence
            // For simplicity, process left-to-right with explicit ops
            TVector<TSharedPtr<IAssetPredicate>> AndGroups;
            TVector<TSharedPtr<IAssetPredicate>> CurrentAndGroup;
        
            CurrentAndGroup.push_back(Predicates[0]);
        
            for (size_t i = 0; i < Ops.size() && i + 1 < Predicates.size(); ++i)
            {
                if (Ops[i] == ELogicalOp::And)
                {
                    CurrentAndGroup.push_back(Predicates[i + 1]);
                }
                else // Or
                {
                    if (CurrentAndGroup.size() == 1)
                    {
                        AndGroups.push_back(CurrentAndGroup[0]);
                    }
                    else
                    {
                        AndGroups.push_back(
                            MakeSharedPtr<FAndPredicate>(Move(CurrentAndGroup)));
                    }

                    CurrentAndGroup.clear();
                    CurrentAndGroup.push_back(Predicates[i + 1]);
                }
            }
        
            // Add final group
            if (CurrentAndGroup.size() == 1)
            {
                AndGroups.push_back(CurrentAndGroup[0]);
            }
            else if (!CurrentAndGroup.empty())
            {
                AndGroups.push_back(
                    MakeSharedPtr<FAndPredicate>(Move(CurrentAndGroup)));
            }

            if (AndGroups.size() == 1)
            {
                return AndGroups[0];
            }

            return MakeSharedPtr<FOrPredicate>(Move(AndGroups));
        }
    
        TSharedPtr<IAssetPredicate> BuildFinalPredicate() const
        {
            return CombinePredicatesWithOps(CurrentPredicates, LogicalOps);
        }
    };
    template<typename AssetType>
    class TAssetQuery : public FAssetQuery
    {
        static_assert(eastl::is_base_of_v<CObject, AssetType>, "AssetType must derive from CObject");
    
    public:
        TAssetQuery()
        {
            OfClass<AssetType>();
        }
    
        // Override return types for method chaining
        TAssetQuery& WithPath(const FString& Path, bool bExact = false, bool bRec = true)
        {
            FAssetQuery::WithPath(Path, bExact, bRec);
            return *this;
        }
    
        TAssetQuery& WithName(const FString& Pattern, bool bCase = false)
        {
            FAssetQuery::WithName(Pattern, bCase);
            return *this;
        }
    
        template<typename Func>
        TAssetQuery& Where(Func&& Lambda, FString Desc = "Custom")
        {
            FAssetQuery::Where(eastl::forward<Func>(Lambda), Move(Desc));
            return *this;
        }
    };



    template<typename T>
    TAssetQuery<T> QueryAssetsOfType()
    {
        return TAssetQuery<T>();
    }

// ============================================================================
// Usage Examples
// ============================================================================

/*
// Example 1: Simple path query
auto Query1 = FAssetQuery()
    .WithPath("/Game/Characters");

// Example 2: Complex boolean logic
auto Query2 = FAssetQuery()
    .OfClass<UStaticMesh>()
    .And()
    .BeginGroup()
        .WithPath("/Game/Props")
        .Or()
        .WithPath("/Game/Environment")
    .EndGroup()
    .And()
    .WithTag("Quality", "High");

// Example 3: Lambda with type deduction
auto Query3 = FAssetQuery()
    .Where([](const FAssetData& Asset) {
        return Asset.AssetName.ToString().length() > 10;
    }, "LongNames");

// Example 4: Type-safe query
auto TypedQuery = QueryAssetsOfType<UStaticMesh>()
    .WithName("SM_")
    .WithPath("/Game/Meshes");

// Example 5: Complex negation
auto Query5 = QUERY_ASSETS()
    .WithPath("/Game")
    .And()
    .Not().WithTag("Deprecated")
    .And()
    .Not().WithTag("Hidden");

// Example 6: Execute and iterate
auto Results = Query5.Execute(AssetRegistry);
for (const FAssetData& Asset : Results)
{
    // Process asset
}

// Example 7: Query inspection
FString QueryStr = Query5.ToString();
size_t Count = Query5.Count(AssetRegistry);
bool HasAny = Query5.Any(AssetRegistry);

// Example 8: Clone and modify query
FAssetQuery ModifiedQuery = Query5.Clone();
ModifiedQuery.WithTag("NewTag", "Value");
*/
}
