#include "pch.h"
#include "World.h"
#include "WorldManager.h"
#include "Core/Delegates/CoreDelegates.h"
#include "Core/Engine/Engine.h"
#include "Core/Object/Class.h"
#include "Core/Object/ObjectIterator.h"
#include "Core/Serialization/MemoryArchiver.h"
#include "Core/Serialization/ObjectArchiver.h"
#include "EASTL/sort.h"
#include "Entity/EntityUtils.h"
#include "Entity/Components/DirtyComponent.h"
#include "Entity/Components/EditorComponent.h"
#include "Entity/Components/InterpolatingMovementComponent.h"
#include "Entity/Components/LineBatcherComponent.h"
#include "Entity/Components/LuaComponent.h"
#include "Entity/Components/NameComponent.h"
#include "Entity/Components/SingletonEntityComponent.h"
#include "Events/KeyCodes.h"
#include "glm/gtx/matrix_decompose.hpp"
#include "Physics/Physics.h"
#include "Scene/RenderScene/Forward/ForwardRenderScene.h"
#include "Scripting/Lua/Scripting.h"
#include "Subsystems/FCameraManager.h"
#include "World/Entity/Components/RelationshipComponent.h"
#include "World/entity/systems/EntitySystem.h"

namespace Lumina
{
    CWorld::CWorld()
        : SingletonEntity(entt::null)
        , SystemContext(this)
    {
    }

    void CWorld::Serialize(FArchive& Ar)
    {
        CObject::Serialize(Ar);
        ECS::Utils::SerializeRegistry(Ar, EntityRegistry);
        ECS::Utils::SerializeEntity(Ar, EntityRegistry, SingletonEntity);
    }

    void CWorld::PreLoad()
    {
        //...
    }

    void CWorld::PostLoad()
    {
        //...
    }
    
    void CWorld::InitializeWorld()
    {
        using namespace entt::literals;
        
        GEngine->GetEngineSubsystem<FWorldManager>()->AddWorld(this);

        EntityRegistry.ctx().emplace<entt::dispatcher&>(SingletonDispatcher);
        
        if (!EntityRegistry.valid(SingletonEntity))
        {
            SingletonEntity = EntityRegistry.create();
        }
        
        EntityRegistry.emplace<FSingletonEntityTag>(SingletonEntity);
        EntityRegistry.emplace<FHideInSceneOutliner>(SingletonEntity);
        EntityRegistry.emplace<FLuaScriptsContainerComponent>(SingletonEntity);

        ScriptUpdatedDelegateHandle = Scripting::FScriptingContext::Get().OnScriptLoaded.AddMember(this, &ThisClass::RegisterSystems);
        
        PhysicsScene    = Physics::GetPhysicsContext()->CreatePhysicsScene(this);
        CameraManager   = MakeUnique<FCameraManager>(this);

        EntityRegistry.ctx().emplace<Physics::IPhysicsScene*>(PhysicsScene.get());
        EntityRegistry.ctx().emplace<FCameraManager*>(CameraManager.get());
        EntityRegistry.ctx().emplace<FSystemContext&>(SystemContext);
        
        CreateRenderer();
        
        RegisterSystems();
        
        EntityRegistry.on_construct<SSineWaveMovementComponent>().connect<&ThisClass::OnSineWaveMovementComponentCreated>(this);
        EntityRegistry.on_destroy<FRelationshipComponent>().connect<&ThisClass::OnRelationshipComponentDestroyed>(this);
        SystemContext.EventSink<FSwitchActiveCameraEvent>().connect<&ThisClass::OnChangeCameraEvent>(this);
    }
    
    void CWorld::Update(const FUpdateContext& Context)
    {
        LUMINA_PROFILE_SCOPE();

        const EUpdateStage Stage = Context.GetUpdateStage();
        
        if (Stage == EUpdateStage::FrameStart)
        {
            DeltaTime = Context.GetDeltaTime();
            TimeSinceCreation += DeltaTime;
        }
        
        if (Stage == EUpdateStage::DuringPhysics)
        {
            PhysicsScene->Update(Context.GetDeltaTime());
        }

        SystemContext.DeltaTime = DeltaTime;
        SystemContext.Time = TimeSinceCreation;
        SystemContext.UpdateStage = Stage;
        
        TickSystems(SystemContext);
    }

    void CWorld::Paused(const FUpdateContext& Context)
    {
        LUMINA_PROFILE_SCOPE();

        DeltaTime = Context.GetDeltaTime();
        TimeSinceCreation += DeltaTime;
        
        SystemContext.DeltaTime = DeltaTime;
        SystemContext.Time = TimeSinceCreation;
        SystemContext.UpdateStage = EUpdateStage::Paused;

        TickSystems(SystemContext);
    }

    void CWorld::Render(FRenderGraph& RenderGraph)
    {
        LUMINA_PROFILE_SCOPE();

        SCameraComponent* CameraComponent = GetActiveCamera();
        FViewVolume ViewVolume = CameraComponent ? CameraComponent->GetViewVolume() : FViewVolume();
        
        RenderScene->RenderScene(RenderGraph, ViewVolume);
    }


    void CWorld::ShutdownWorld()
    {
        EntityRegistry.on_construct<SSineWaveMovementComponent>().disconnect<&ThisClass::OnSineWaveMovementComponentCreated>(this);
        EntityRegistry.on_destroy<FRelationshipComponent>().disconnect<&ThisClass::OnRelationshipComponentDestroyed>(this);
        
        EntityRegistry.clear<>();
        
        //ForEachUniqueSystem([&](const FSystemVariant& System)
        //{
        //   System->Shutdown(SystemContext); 
        //});
        
        Scripting::FScriptingContext::Get().OnScriptLoaded.Remove(ScriptUpdatedDelegateHandle);

        PhysicsScene.reset();
        DestroyRenderer();
        
        FCoreDelegates::PostWorldUnload.Broadcast();
        
        GEngine->GetEngineSubsystem<FWorldManager>()->RemoveWorld(this);
    }

    bool CWorld::RegisterSystem(const FSystemVariant& NewSystem)
    {
        const FUpdatePriorityList& PriorityList = eastl::visit([&](const auto& System) { return System.GetUpdatePriorityList(); }, NewSystem);
        
        bool StagesModified[(uint8)EUpdateStage::Max] = {};

        for (uint8 i = 0; i < (uint8)EUpdateStage::Max; ++i)
        {
            if (PriorityList.IsStageEnabled((EUpdateStage)i))
            {
                SystemUpdateList[i].emplace_back(NewSystem);
                StagesModified[i] = true;
            }
        }

        for (uint8 i = 0; i < (uint8)EUpdateStage::Max; ++i)
        {
            if (!StagesModified[i])
            {
                continue;
            }

            auto Predicate = [i](const FSystemVariant& A, const FSystemVariant& B)
            {
                const FUpdatePriorityList& PrioListA = eastl::visit([&](const auto& System) { return System.GetUpdatePriorityList(); }, A);
                const FUpdatePriorityList& PrioListB = eastl::visit([&](const auto& System) { return System.GetUpdatePriorityList(); }, B);
                const uint8 PriorityA = PrioListA.GetPriorityForStage((EUpdateStage)i);
                const uint8 PriorityB = PrioListB.GetPriorityForStage((EUpdateStage)i);
                return PriorityA > PriorityB;
            };

            eastl::sort(SystemUpdateList[i].begin(), SystemUpdateList[i].end(), Predicate);
        }

        return true;
    }

    entt::entity CWorld::ConstructEntity(const FName& Name, const FTransform& Transform)
    {
        entt::entity NewEntity = GetEntityRegistry().create();
        
        FName ActualName = Name;
        if (ActualName == NAME_None)
        {
            FFixedString StringName;
            StringName.append_convert(Name + eastl::to_string(entt::to_integral(NewEntity)));
            ActualName = StringName;
        }
        
        EntityRegistry.emplace<SNameComponent>(NewEntity).Name = Name;
        EntityRegistry.emplace<STransformComponent>(NewEntity).Transform = Transform;
        EntityRegistry.emplace_or_replace<FNeedsTransformUpdate>(NewEntity);
        
        return NewEntity;
    }
    
    void CWorld::CopyEntity(entt::entity& To, entt::entity From, TFunctionRef<bool(entt::type_info)> Callback)
    {
        ASSERT(To != From);
        
        To = EntityRegistry.create();
        
        for (auto [ID, Storage]: EntityRegistry.storage())
        {
            if (!Callback(Storage.info()))
            {
                continue;
            }
            
            // We also need to check the entity we're creating, incase another component adds it.
            if(Storage.contains(From) && !Storage.contains(To))
            {
                Storage.push(To, Storage.value(From));
            }
        }

        FString OldName = EntityRegistry.get<SNameComponent>(From).Name.ToString();

        FString BaseName = OldName;
        size_t Pos = OldName.find_last_of('_');
        if (Pos != FString::npos && Pos + 1 < OldName.size())
        {
            BaseName = OldName.substr(0, Pos);
        }

        FString NewName = BaseName + "_" +  eastl::to_string(entt::to_integral(To));
        EntityRegistry.get<SNameComponent>(To).Name = NewName;
    }

    void CWorld::DestroyEntity(entt::entity Entity)
    {
        EntityRegistry.destroy(Entity);
    }

    uint32 CWorld::GetNumEntities() const
    {
        return (uint32)EntityRegistry.view<entt::entity>().size();
    }

    void CWorld::SetActiveCamera(entt::entity InEntity)
    {
        if (!EntityRegistry.valid(InEntity))
        {
            return;
        }

        if (EntityRegistry.all_of<SCameraComponent>(InEntity))
        {
            CameraManager->SetActiveCamera(InEntity);
        }
    }

    SCameraComponent* CWorld::GetActiveCamera()
    {
        return CameraManager->GetCameraComponent();
    }

    entt::entity CWorld::GetActiveCameraEntity() const
    {
        return CameraManager->GetActiveCameraEntity();
    }

    void CWorld::OnChangeCameraEvent(const FSwitchActiveCameraEvent& Event)
    {
        SetActiveCamera(Event.NewActiveEntity);
    }

    void CWorld::BeginPlay()
    {
        PhysicsScene->OnWorldSimulate();
    }

    void CWorld::EndPlay()
    {
        PhysicsScene->OnWorldStopSimulate();
    }

    void CWorld::CreateRenderer()
    {
        if (!RenderScene)
        {
            RenderScene = MakeUnique<FForwardRenderScene>(this);
            RenderScene->Init();
            EntityRegistry.ctx().emplace<IRenderScene*>(RenderScene.get());
        }
    }

    void CWorld::DestroyRenderer()
    {
        if (RenderScene)
        {
            RenderScene->Shutdown();
            RenderScene.reset();
        }
    }

    void CWorld::SetActive(bool bNewActive)
    {
        if (bActive != bNewActive)
        {
            bActive = bNewActive;
            
            if (bActive)
            {
                CreateRenderer();       
            }
            else
            {
                DestroyRenderer();
            }
        }
    }

    void CWorld::SetSimulating(bool bSim)
    {
        bSimulating = bSim;
        if (bSimulating)
        {
            PhysicsScene->OnWorldSimulate();
        }
        else
        {
            PhysicsScene->OnWorldStopSimulate();
        }
    }

    CWorld* CWorld::DuplicateWorld(CWorld* OwningWorld)
    {
        CPackage* OuterPackage = OwningWorld->GetPackage();
        if (OuterPackage == nullptr)
        {
            return nullptr;
        }

        TVector<uint8> Data;
        FMemoryWriter Writer(Data);
        FObjectProxyArchiver WriterProxy(Writer, true);
        OwningWorld->Serialize(WriterProxy);
        
        FMemoryReader Reader(Data);
        FObjectProxyArchiver ReaderProxy(Reader, true);
        
        CWorld* PIEWorld = NewObject<CWorld>(OF_Transient);
        PIEWorld->InitializeWorld();
        
        PIEWorld->PreLoad();
        PIEWorld->Serialize(ReaderProxy);
        PIEWorld->PostLoad();
        
        return PIEWorld;
    }

    const TVector<CWorld::FSystemVariant>& CWorld::GetSystemsForUpdateStage(EUpdateStage Stage)
    {
        return SystemUpdateList[static_cast<uint32>(Stage)];
    }

    void CWorld::OnRelationshipComponentDestroyed(entt::registry& Registry, entt::entity Entity)
    {
        ECS::Utils::RemoveFromParent(Registry, Entity);
        ECS::Utils::DestroyEntityHierarchy(Registry, Entity);
    }

    void CWorld::OnSineWaveMovementComponentCreated(entt::registry& Registry, entt::entity Entity)
    {
        SSineWaveMovementComponent& MovementComponent = Registry.get<SSineWaveMovementComponent>(Entity);
        MovementComponent.InitialPosition = Registry.get<STransformComponent>(Entity).GetLocation();
    }

    void CWorld::RegisterSystems()
    {
        using namespace Scripting;
        using namespace entt::literals;

        for (int i = 0; i < (int)EUpdateStage::Max; ++i)
        {
            SystemUpdateList[i].clear();
        }
        
        for (auto&& [_, Meta] : entt::resolve())
        {
            ECS::ETraits Traits = Meta.traits<ECS::ETraits>();
            if (EnumHasAnyFlags(Traits, ECS::ETraits::System))
            {
                FEntitySystemWrapper Wrapper;
                Wrapper.Underlying = Meta;
                Wrapper.Instance = Meta.construct();
                
                FSystemVariant Variant = Wrapper;
                RegisterSystem(Variant);
            }
        }
        
        FScriptingContext::Get().ForEachScript<FLuaSystemScriptEntry>([&](const FLuaSystemScriptEntry& Script)
        {
            FEntityScriptSystem ScriptSystem;
            ScriptSystem.ScriptSystem = Script; //@TODO Figure out if this needs to be copied or not.
            FSystemVariant Variant = Move(ScriptSystem);
            RegisterSystem(Variant);
        });
    }

    void CWorld::DrawBillboard(FRHIImage* Image, const glm::vec3& Location, float Scale)
    {
        RenderScene->DrawBillboard(Image, Location, Scale);
    }

    void CWorld::DrawLine(const glm::vec3& Start, const glm::vec3& End, const glm::vec4& Color, float Thickness, bool bDepthTest, float Duration)
    {
        FLineBatcherComponent& Batcher = GetOrCreateLineBatcher();
        
        float ActualDuration = eastl::max<float>(static_cast<float>(GetWorldDeltaTime()) + LE_KINDA_SORTA_SMALL_NUMBER, Duration);
        Batcher.DrawLine(Start, End, Color, Thickness, bDepthTest, ActualDuration);
    }
    
    TOptional<FRayResult> CWorld::CastRay(const FRayCastSettings& Settings)
    {
        LUMINA_PROFILE_SCOPE();
        
        if (PhysicsScene == nullptr)
        {
            return eastl::nullopt;
        }
        
        TOptional<FRayResult> Result = PhysicsScene->CastRay(Settings);
        
        if (Settings.bDrawDebug)
        {
            if (Result.has_value())
            {
                FRayResult RayResult = Result.value();
                DrawLine(Settings.Start, RayResult.Location, FColor(Settings.DebugMissColor), 1.0f, true, Settings.DebugDuration);
                
                glm::vec3 NormalEnd = RayResult.Location + RayResult.Normal * 0.5f;
                DrawLine(RayResult.Location, NormalEnd, FColor::Blue, 1.0f,true, Settings.DebugDuration);
                
                DrawBox(RayResult.Location, glm::vec3(0.05f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), FColor::Yellow, 1.0, true, Settings.DebugDuration);
                
                DrawLine(RayResult.Location, Settings.End, FColor(Settings.DebugHitColor), 1.0f, true, Settings.DebugDuration);
            }
            else
            {
                DrawLine(Settings.Start, Settings.End, FColor(Settings.DebugMissColor), 1.0f, true, Settings.DebugDuration);
            }
        }
        
        return Move(Result);
    }

    TOptional<FRayResult> CWorld::CastRay(const glm::vec3& Start, const glm::vec3& End, bool bDrawDebug, float DebugDuration, uint32 LayerMask, int64 IgnoreBody)
    {
        FRayCastSettings Settings;
        Settings.DebugDuration = DebugDuration;
        Settings.Start = Start;
        Settings.End = End;
        Settings.LayerMask = LayerMask;
        Settings.bDrawDebug = bDrawDebug;
        Settings.IgnoreBodies.push_back(IgnoreBody);
        
        return CastRay(Settings);
    }
    
    TVector<FRayResult> CWorld::CastSphere(const FSphereCastSettings& Settings)
    {
        LUMINA_PROFILE_SCOPE();

        if (PhysicsScene == nullptr)
        {
            return {};
        }
        
        return PhysicsScene->CastSphere(Settings);
        
    }

    void CWorld::MarkTransformDirty(entt::entity Entity)
    {
        GetEntityRegistry().emplace_or_replace<FNeedsTransformUpdate>(Entity);
    }

    void CWorld::SetEntityTransform(entt::entity Entity, const FTransform& NewTransform)
    {
        EntityRegistry.emplace_or_replace<STransformComponent>(Entity, NewTransform);
        EntityRegistry.emplace_or_replace<FNeedsTransformUpdate>(Entity);
    }

    TVector<entt::entity> CWorld::GetSelectedEntities() const
    {
        auto View = EntityRegistry.view<FSelectedInEditorComponent>();
        TVector<entt::entity> Selections(View.size());
        View.each([&](entt::entity Entity)
        {
           Selections.push_back(Entity); 
        });
        return Selections;
    }

    bool CWorld::IsSelected(entt::entity Entity) const
    {
        return EntityRegistry.any_of<FSelectedInEditorComponent>(Entity);
    }

    void CWorld::TickSystems(FSystemContext& Context)
    {
        auto& SystemVector = SystemUpdateList[(uint32)Context.GetUpdateStage()];
        for(FSystemVariant& SystemVariant : SystemVector)
        {
            eastl::visit([&](auto& System) { System.Update(Context); }, SystemVariant);
        }
    }

    FLineBatcherComponent& CWorld::GetOrCreateLineBatcher()
    {
        return EntityRegistry.get_or_emplace<FLineBatcherComponent>(SingletonEntity);
    }
}
