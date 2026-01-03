#include "pch.h"
#include "World.h"

#include "imgui.h"
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
#include "Entity/Systems/ScriptEntitySystem.h"
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
        GEngine->GetEngineSubsystem<FWorldManager>()->AddWorld(this);

        EntityRegistry.ctx().emplace<entt::dispatcher&>(SingletonDispatcher);
        
        if (!EntityRegistry.valid(SingletonEntity))
        {
            SingletonEntity = EntityRegistry.create();
        }
        
        EntityRegistry.emplace<FSingletonEntityTag>(SingletonEntity);
        EntityRegistry.emplace<FHideInSceneOutliner>(SingletonEntity);
        EntityRegistry.emplace<FLuaScriptsContainerComponent>(SingletonEntity);

        ScriptUpdatedDelegateHandle = Scripting::FScriptingContext::Get().OnScriptLoaded.AddMember(this, &ThisClass::ProcessAnyNewlyLoadedScripts);
        
        PhysicsScene    = Physics::GetPhysicsContext()->CreatePhysicsScene(this);
        CameraManager   = MakeUniquePtr<FCameraManager>(this);
        RenderScene     = MakeUniquePtr<FForwardRenderScene>(this);

        EntityRegistry.ctx().emplace<Physics::IPhysicsScene*>(PhysicsScene.get());
        EntityRegistry.ctx().emplace<FCameraManager*>(CameraManager.get());
        EntityRegistry.ctx().emplace<IRenderScene*>(RenderScene.get());
        EntityRegistry.ctx().emplace<FSystemContext&>(SystemContext);

        RenderScene->Init();
        
        ProcessAnyNewlyLoadedScripts();
        
        TVector<TObjectPtr<CEntitySystem>> Systems;
        CEntitySystemRegistry::Get().GetRegisteredSystems(Systems);
        for (CEntitySystem* System : Systems)
        {
            if (System->GetRequiredUpdatePriorities())
            {
                RegisterSystem(System);
            }
        }

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
        
        auto& SystemVector = SystemUpdateList[(uint32)Stage];
        for(CEntitySystem* System : SystemVector)
        {
            System->Update(SystemContext);
        }
        
        UpdateScripts();
    }

    void CWorld::Paused(const FUpdateContext& Context)
    {
        LUMINA_PROFILE_SCOPE();

        DeltaTime = Context.GetDeltaTime();
        TimeSinceCreation += DeltaTime;
        
        SystemContext.DeltaTime = DeltaTime;
        SystemContext.Time = TimeSinceCreation;
        SystemContext.UpdateStage = EUpdateStage::Paused;

        auto& SystemVector = SystemUpdateList[(uint32)EUpdateStage::Paused];
        for(CEntitySystem* System : SystemVector)
        {
            System->Update(SystemContext);
        }
        
        UpdateScripts();
    }

    void CWorld::Render(FRenderGraph& RenderGraph)
    {
        LUMINA_PROFILE_SCOPE();

        SCameraComponent* CameraComponent = GetActiveCamera();
        FViewVolume ViewVolume = CameraComponent ? CameraComponent->GetViewVolume() : FViewVolume();
        
        RenderScene->RenderScene(RenderGraph, ViewVolume);
    }

    void CWorld::UpdateScripts()
    {
    }

    void CWorld::ShutdownWorld()
    {
        EntityRegistry.on_construct<SSineWaveMovementComponent>().disconnect<&ThisClass::OnSineWaveMovementComponentCreated>(this);
        EntityRegistry.on_destroy<FRelationshipComponent>().disconnect<&ThisClass::OnRelationshipComponentDestroyed>(this);
        
        EntityRegistry.clear<>();
        
        ForEachUniqueSystem([&](CEntitySystem* System)
        {
           System->Shutdown(SystemContext); 
        });
        
        Scripting::FScriptingContext::Get().OnScriptLoaded.Remove(ScriptUpdatedDelegateHandle);
        
        PhysicsScene.reset();
        RenderScene->Shutdown();
        
        FCoreDelegates::PostWorldUnload.Broadcast();
        
        GEngine->GetEngineSubsystem<FWorldManager>()->RemoveWorld(this);
    }

    bool CWorld::RegisterSystem(CEntitySystem* NewSystem)
    {
        Assert(NewSystem != nullptr)

        NewSystem->Init(SystemContext);
        
        bool StagesModified[(uint8)EUpdateStage::Max] = {};

        for (uint8 i = 0; i < (uint8)EUpdateStage::Max; ++i)
        {
            if (NewSystem->GetRequiredUpdatePriorities()->IsStageEnabled((EUpdateStage)i))
            {
                SystemUpdateList[i].push_back(NewSystem);
                StagesModified[i] = true;
            }
        }

        for (uint8 i = 0; i < (uint8)EUpdateStage::Max; ++i)
        {
            if (!StagesModified[i])
            {
                continue;
            }

            auto Predicate = [i](CEntitySystem* A, CEntitySystem* B)
            {
                const uint8 PriorityA = A->GetRequiredUpdatePriorities()->GetPriorityForStage((EUpdateStage)i);
                const uint8 PriorityB = B->GetRequiredUpdatePriorities()->GetPriorityForStage((EUpdateStage)i);
                return PriorityA > PriorityB;
            };

            eastl::sort(SystemUpdateList[i].begin(), SystemUpdateList[i].end(), Predicate);
        }

        return true;
    }

    entt::entity CWorld::ConstructEntity(const FName& Name, const FTransform& Transform)
    {
        entt::entity NewEntity = GetEntityRegistry().create();

        FString StringName(Name.c_str());
        StringName += "_" + eastl::to_string(static_cast<int>(NewEntity));
        
        EntityRegistry.emplace<SNameComponent>(NewEntity).Name = StringName;
        EntityRegistry.emplace<STransformComponent>(NewEntity).Transform = Transform;
        EntityRegistry.emplace_or_replace<FNeedsTransformUpdate>(NewEntity);
        
        return NewEntity;
    }
    
    void CWorld::CopyEntity(entt::entity& To, entt::entity From)
    {
        LUM_ASSERT(To != From)
        
        To = EntityRegistry.create();
        
        for (auto [ID, Storage]: EntityRegistry.storage())
        {
            if (Storage.info() == entt::type_id<FRelationshipComponent>() || Storage.info() == entt::type_id<FSelectedInEditorComponent>())
            {
                continue;
            }
            
            if(Storage.contains(From))
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

    const TVector<CEntitySystem*>& CWorld::GetSystemsForUpdateStage(EUpdateStage Stage)
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

    void CWorld::ProcessAnyNewlyLoadedScripts()
    {
        using namespace Scripting;
        using namespace entt::literals;
        
        auto View = EntityRegistry.view<FLuaScriptsContainerComponent>();
        View.each([&](FLuaScriptsContainerComponent& LuaContainerComponent)
        {
            for (uint32 i = 0; i < static_cast<uint32>(EUpdateStage::Max); ++i)
            {
                LuaContainerComponent.Systems[i].clear();
            }
            
            FScriptingContext::Get().ForEachScript<FLuaSystemScriptEntry>([&](FLuaSystemScriptEntry& Script)
            {
                if (!Script.bEnabled)
                {
                    return;
                }
                
                LuaContainerComponent.Systems[Script.Stage].emplace_back(Script);
            });

            // Push updates to initialized lua files.
            GetMutableDefault<CScriptEntitySystem>()->Init(SystemContext);
        });
    }

    void CWorld::DrawLine(const glm::vec3& Start, const glm::vec3& End, const glm::vec4& Color, float Thickness, float Duration)
    {
        FLineBatcherComponent& Batcher = GetOrCreateLineBatcher();
        
        float ActualDuration = eastl::max<float>(static_cast<float>(GetWorldDeltaTime()) + LE_KINDA_SORTA_SMALL_NUMBER, Duration);
        Batcher.DrawLine(Start, End, Color, Thickness, ActualDuration);
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
                DrawLine(Settings.Start, RayResult.Location, FColor(Settings.DebugMissColor), 1.0f, Settings.DebugDuration);
                
                glm::vec3 NormalEnd = RayResult.Location + RayResult.Normal * 0.5f;
                DrawLine(RayResult.Location, NormalEnd, FColor::Blue, 1.0f, Settings.DebugDuration);
                
                DrawBox(RayResult.Location, glm::vec3(0.05f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), FColor::Yellow, 1.0, Settings.DebugDuration);
                
                DrawLine(RayResult.Location, Settings.End, FColor(Settings.DebugHitColor), 1.0f, Settings.DebugDuration);
            }
            else
            {
                DrawLine(Settings.Start, Settings.End, FColor(Settings.DebugMissColor), 1.0f, Settings.DebugDuration);
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

    void CWorld::SetSelectedEntity(entt::entity EntityID)
    {
        EntityRegistry.clear<FSelectedInEditorComponent>();
        
        if (EntityRegistry.valid(EntityID))
        {
            EntityRegistry.emplace<FSelectedInEditorComponent>(EntityID);
        }
    }

    entt::entity CWorld::GetSelectedEntity() const
    {
        auto View = EntityRegistry.view<FSelectedInEditorComponent>();
        LUM_ASSERT(View.size() <= 1)

        for (entt::entity Entity : View)
        {
            return Entity;
        }

        return entt::null;
    }

    FLineBatcherComponent& CWorld::GetOrCreateLineBatcher()
    {
        return EntityRegistry.get_or_emplace<FLineBatcherComponent>(SingletonEntity);
    }
}
