#pragma once

#include "AssetData.h"
#include "Assets/AssetHeader.h"
#include "Core/DisableAllWarnings.h"
#include "Subsystems/Subsystem.h"
#include "Core/Delegates/Delegate.h"
#include "Core/Serialization/MemoryArchiver.h"
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
			size_t Seed;
			Hash::HashCombine(Seed, Asset->AssetGUID);
			
			return Seed;
		}
	};

	struct FAssetDataPtrEqual
	{
		bool operator()(const TSharedPtr<FAssetData>& A, const TSharedPtr<FAssetData>& B) const noexcept
		{
			return A->AssetGUID == B->AssetGUID;
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
		void AssetDeleted(const FName& Package);
		void AssetRenamed(CObject* Asset, const FString& OldPackagePath);
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
        virtual eastl::shared_ptr<IAssetPredicate> Clone() const = 0;
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
            : Path(eastl::move(InPath)), bExactMatch(bExact), bRecursive(bRec) {}
    
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
    
        eastl::shared_ptr<IAssetPredicate> Clone() const override
        {
            return eastl::make_shared<FPathPredicate>(Path, bExactMatch, bRecursive);
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
    
        eastl::shared_ptr<IAssetPredicate> Clone() const override
        {
            return eastl::make_shared<FGuidPredicate>(Guid);
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
    
        eastl::shared_ptr<IAssetPredicate> Clone() const override
        {
            return eastl::make_shared<FClassPredicate>(ClassName, bIncludeDerived);
        }
    };

    class LUMINA_API FNamePredicate : public IAssetPredicate
    {
        FString NamePattern;
        bool bCaseSensitive;
    
    public:
        explicit FNamePredicate(FString InPattern, bool bCase = false)
            : NamePattern(eastl::move(InPattern)), bCaseSensitive(bCase) {}
    
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
    
        eastl::shared_ptr<IAssetPredicate> Clone() const override
        {
            return eastl::make_shared<FNamePredicate>(NamePattern, bCaseSensitive);
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
            : Lambda(eastl::forward<Func>(InLambda)), Description(eastl::move(InDesc)) {}
    
        bool Evaluate(const FAssetData& Asset) const override
        {
            return Lambda(Asset);
        }
    
        FString ToString() const override
        {
            return FString().sprintf("Lambda(%s)", Description.c_str());
        }
    
        eastl::shared_ptr<IAssetPredicate> Clone() const override
        {
            return eastl::make_shared<TLambdaPredicate<Func>>(eastl::forward<Func>(const_cast<Func&>(Lambda)), Description);
        }
    };

    // Helper to create lambda predicates with type deduction
    template<typename Func>
    eastl::shared_ptr<IAssetPredicate> MakeLambdaPredicate(Func&& Lambda, FString Desc = "Custom")
    {
        return eastl::make_shared<TLambdaPredicate<eastl::decay_t<Func>>>(eastl::forward<Func>(Lambda), eastl::move(Desc));
    }

    // ============================================================================
    // Logical Combinators with SFINAE
    // ============================================================================

    class LUMINA_API FAndPredicate : public IAssetPredicate
    {
        eastl::vector<eastl::shared_ptr<IAssetPredicate>> Predicates;
    
    public:
        explicit FAndPredicate(eastl::vector<eastl::shared_ptr<IAssetPredicate>> InPredicates)
            : Predicates(eastl::move(InPredicates)) {}
    
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
    
        eastl::shared_ptr<IAssetPredicate> Clone() const override
        {
            eastl::vector<eastl::shared_ptr<IAssetPredicate>> ClonedPreds;
            ClonedPreds.reserve(Predicates.size());
            for (const auto& P : Predicates)
            {
                ClonedPreds.push_back(P->Clone());
            }
            return eastl::make_shared<FAndPredicate>(eastl::move(ClonedPreds));
        }
    };
    
    class LUMINA_API FOrPredicate : public IAssetPredicate
    {
        eastl::vector<eastl::shared_ptr<IAssetPredicate>> Predicates;
    
    public:
        explicit FOrPredicate(eastl::vector<eastl::shared_ptr<IAssetPredicate>> InPredicates)
            : Predicates(eastl::move(InPredicates)) {}
    
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
    
        eastl::shared_ptr<IAssetPredicate> Clone() const override
        {
            eastl::vector<eastl::shared_ptr<IAssetPredicate>> ClonedPreds;
            ClonedPreds.reserve(Predicates.size());
            for (const auto& P : Predicates)
            {
                ClonedPreds.push_back(P->Clone());
            }
            return eastl::make_shared<FOrPredicate>(eastl::move(ClonedPreds));
        }
    };

    class LUMINA_API FNotPredicate : public IAssetPredicate
    {
        eastl::shared_ptr<IAssetPredicate> Predicate;
    
    public:
        explicit FNotPredicate(eastl::shared_ptr<IAssetPredicate> InPredicate)
            : Predicate(eastl::move(InPredicate)) {}
    
        bool Evaluate(const FAssetData& Asset) const override
        {
            return Predicate ? !Predicate->Evaluate(Asset) : true;
        }
    
        FString ToString() const override
        {
            return FString().sprintf("NOT(%s)", Predicate->ToString().c_str());
        }
    
        eastl::shared_ptr<IAssetPredicate> Clone() const override
        {
            return eastl::make_shared<FNotPredicate>(Predicate->Clone());
        }
    };

    // ============================================================================
    // Query Builder with Fluent Interface and Expression Templates
    // ============================================================================

    class LUMINA_API FAssetQuery
    {
    public:
        FAssetQuery() = default;
        FAssetQuery(const FAssetQuery&) = default;
        FAssetQuery(FAssetQuery&&) = default;
        FAssetQuery& operator=(const FAssetQuery&) = default;
        FAssetQuery& operator=(FAssetQuery&&) noexcept = default;
    
        // ========================================================================
        // Predicate Builders
        // ========================================================================
    
        FAssetQuery& WithPath(const FString& Path, bool bExact = false, bool bRecursive = true)
        {
            AddPredicate(eastl::make_shared<FPathPredicate>(Path, bExact, bRecursive));
            return *this;
        }
    
        FAssetQuery& WithGuid(const FGuid& Guid)
        {
            AddPredicate(eastl::make_shared<FGuidPredicate>(Guid));
            return *this;
        }
    
        FAssetQuery& OfClass(FName ClassName, bool bIncludeDerived = true)
        {
            AddPredicate(eastl::make_shared<FClassPredicate>(ClassName, bIncludeDerived));
            return *this;
        }
    
        // Template version for compile-time class checking
        template<typename T>
        FAssetQuery& OfClass(bool bIncludeDerived = true)
        {
            static_assert(eastl::is_base_of_v<CObject, T>, "T must derive from CObject");
            return OfClass(T::StaticClass()->GetFName(), bIncludeDerived);
        }
    
        FAssetQuery& WithName(const FString& Pattern, bool bCaseSensitive = false)
        {
            AddPredicate(eastl::make_shared<FNamePredicate>(Pattern, bCaseSensitive));
            return *this;
        }
    
        FAssetQuery& WithTag(FName Key, const FString& Value = FString())
        {
            //AddPredicate(eastl::make_shared<FTagPredicate>(Key, Value));
            return *this;
        }
    
        template<typename Func>
        FAssetQuery& Where(Func&& Lambda, FString Description = "Custom")
        {
            static_assert(eastl::is_invocable_r_v<bool, Func, const FAssetData&>, "Lambda must be callable with signature: bool(const FAssetData&)");
        
            AddPredicate(MakeLambdaPredicate(eastl::forward<Func>(Lambda), 
                eastl::move(Description)));
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
            GroupStack.push_back(eastl::move(CurrentPredicates));
            CurrentPredicates.clear();
            LogicalOpStack.push_back(eastl::move(LogicalOps));
            LogicalOps.clear();
            return *this;
        }
    
        FAssetQuery& EndGroup()
        {
            if (!GroupStack.empty())
            {
                auto GroupPredicates = eastl::move(CurrentPredicates);
                CurrentPredicates = eastl::move(GroupStack.back());
                GroupStack.pop_back();
            
                auto GroupOps = eastl::move(LogicalOps);
                LogicalOps = eastl::move(LogicalOpStack.back());
                LogicalOpStack.pop_back();
            
                if (!GroupPredicates.empty())
                {
                    auto GroupPred = CombinePredicatesWithOps(eastl::move(GroupPredicates), eastl::move(GroupOps));
                    AddPredicate(eastl::move(GroupPred));
                }
            }
            return *this;
        }
    
        // ========================================================================
        // Execution Methods
        // ========================================================================
    
        eastl::vector<FAssetData> Execute() const
        {
            FAssetRegistry& Registry = FAssetRegistry::Get();
            eastl::vector<FAssetData> Results;
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
    
        eastl::vector<eastl::shared_ptr<IAssetPredicate>> CurrentPredicates;
        eastl::vector<ELogicalOp> LogicalOps;
        eastl::vector<eastl::vector<eastl::shared_ptr<IAssetPredicate>>> GroupStack;
        eastl::vector<eastl::vector<ELogicalOp>> LogicalOpStack;
        bool bNegateNext = false;
    
        void AddPredicate(eastl::shared_ptr<IAssetPredicate> Predicate)
        {
            if (bNegateNext)
            {
                Predicate = eastl::make_shared<FNotPredicate>(eastl::move(Predicate));
                bNegateNext = false;
            }
            CurrentPredicates.push_back(eastl::move(Predicate));
        }
    
        void PushLogicalOperator(ELogicalOp Op)
        {
            LogicalOps.push_back(Op);
        }
    
        eastl::shared_ptr<IAssetPredicate> CombinePredicatesWithOps(
            eastl::vector<eastl::shared_ptr<IAssetPredicate>> Predicates,
            eastl::vector<ELogicalOp> Ops) const
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
                            eastl::make_shared<FAndPredicate>(eastl::move(CurrentAndGroup)));
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
                    eastl::make_shared<FAndPredicate>(eastl::move(CurrentAndGroup)));
            }

            if (AndGroups.size() == 1)
            {
                return AndGroups[0];
            }

            return eastl::make_shared<FOrPredicate>(eastl::move(AndGroups));
        }
    
        eastl::shared_ptr<IAssetPredicate> BuildFinalPredicate() const
        {
            return CombinePredicatesWithOps(CurrentPredicates, LogicalOps);
        }
    };

    // ============================================================================
    // Typed Query Builder with Compile-Time Class Constraints
    // ============================================================================

    template<typename AssetType>
    class TAssetQuery : public FAssetQuery
    {
        static_assert(eastl::is_base_of_v<CObject, AssetType>, 
            "AssetType must derive from CObject");
    
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
            FAssetQuery::Where(eastl::forward<Func>(Lambda), eastl::move(Desc));
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
