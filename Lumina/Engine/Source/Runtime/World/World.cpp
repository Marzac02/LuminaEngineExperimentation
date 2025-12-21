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
#include "Entity/Components/VelocityComponent.h"
#include "Events/KeyCodes.h"
#include "glm/gtx/matrix_decompose.hpp"
#include "Physics/Physics.h"
#include "Scene/RenderScene/Forward/ForwardRenderScene.h"
#include "Subsystems/FCameraManager.h"
#include "World/Entity/Components/RelationshipComponent.h"
#include "World/entity/systems/EntitySystem.h"
#include "World/Scene/RenderScene/Deferred/DeferredRenderScene.h"

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

        SingletonEntity = EntityRegistry.create();
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
        EntityRegistry.ctx().emplace<entt::dispatcher&>(SingletonDispatcher);

        RenderScene->Init();
        
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
        
        ProcessAnyNewlyLoadedScripts();
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
        Scripting::FScriptingContext::Get().OnScriptLoaded.Remove(ScriptUpdatedDelegateHandle);

        RenderScene->Shutdown();
        
        FCoreDelegates::PostWorldUnload.Broadcast();
        
        GEngine->GetEngineSubsystem<FWorldManager>()->RemoveWorld(this);
    }

    bool CWorld::RegisterSystem(CEntitySystem* NewSystem)
    {
        Assert(NewSystem != nullptr)

        NewSystem->RegisterEventListeners(SystemContext);
        
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
        StringName += "_" + eastl::to_string((int)NewEntity);
        
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
            if (Storage.type() == entt::type_id<FRelationshipComponent>() || Storage.type() == entt::type_id<FSelectedInEditorComponent>())
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

        FString NewName = BaseName + "_" +  eastl::to_string((uint64)To);
        EntityRegistry.get<SNameComponent>(To).Name = NewName;
    }

    void CWorld::ReparentEntity(entt::entity Child, entt::entity Parent)
    {
        if (Child == Parent)
        {
            LOG_ERROR("Cannot parent an entity to itself!");
            return;
        }

        if (Child == entt::null || Parent == entt::null)
        {
            LOG_ERROR("Cannot parent an entity from/to a null!");
            return;
        }

        
        glm::mat4 ChildWorldMatrix = EntityRegistry.get<STransformComponent>(Child).WorldTransform.GetMatrix();
    
        glm::mat4 ParentWorldMatrix = glm::mat4(1.0f);
        if (Parent != entt::null)
        {
            ParentWorldMatrix = EntityRegistry.get<STransformComponent>(Parent).WorldTransform.GetMatrix();
        }
    
        glm::mat4 NewLocalMatrix = glm::inverse(ParentWorldMatrix) * ChildWorldMatrix;
    
        glm::vec3 Translation, Scale, Skew;
        glm::quat Rotation;
        glm::vec4 Perspective;
    
        glm::decompose(NewLocalMatrix, Scale, Rotation, Translation, Skew, Perspective);
    
        FRelationshipComponent& ParentRelationshipComponent = EntityRegistry.get_or_emplace<FRelationshipComponent>(Parent);
        FRelationshipComponent& ChildRelationshipComponent = EntityRegistry.get_or_emplace<FRelationshipComponent>(Child);
    
        if (ParentRelationshipComponent.NumChildren >= FRelationshipComponent::MaxChildren)
        {
            LOG_ERROR("Parent has reached its max children");
            return;
        }
    
        if (ChildRelationshipComponent.Parent != entt::null)
        {
            if (FRelationshipComponent* ToRemove = EntityRegistry.try_get<FRelationshipComponent>(ChildRelationshipComponent.Parent))
            {
                for (SIZE_T i = 0; i < ToRemove->NumChildren; ++i)
                {
                    if (ToRemove->Children[i] == Child)
                    {
                        for (SIZE_T j = i; j < ToRemove->NumChildren - 1; ++j)
                        {
                            ToRemove->Children[j] = ToRemove->Children[j + 1];
                        }
    
                        --ToRemove->NumChildren;
                        break;
                    }
                }
            }
        }
    
        ParentRelationshipComponent.Children[ParentRelationshipComponent.NumChildren++] = Child;
        ChildRelationshipComponent.Parent = Parent;

        
        FTransform NewTransform;
        NewTransform.Location = Translation;
        NewTransform.Rotation = Rotation;
        NewTransform.Scale = Scale;
        
        SetEntityTransform(Child, NewTransform);
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
            return;
        }

        if (FRelationshipComponent* Relationship = EntityRegistry.try_get<FRelationshipComponent>(InEntity))
        {
            for (uint32 i = 0; i < Relationship->NumChildren; ++i)
            {
                entt::entity Child = Relationship->Children[i];
                SetActiveCamera(Child);
            }
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

    const TVector<TObjectPtr<CEntitySystem>>& CWorld::GetSystemsForUpdateStage(EUpdateStage Stage)
    {
        return SystemUpdateList[uint32(Stage)];
    }

    void CWorld::OnRelationshipComponentDestroyed(entt::registry& Registry, entt::entity Entity)
    {
        FRelationshipComponent& ThisRelationship = Registry.get<FRelationshipComponent>(Entity);

        if (ThisRelationship.Parent != entt::null)
        {
            FRelationshipComponent& ParentRelationship = Registry.get<FRelationshipComponent>(ThisRelationship.Parent);
            
            for (SIZE_T i = 0; i < ParentRelationship.NumChildren; ++i)
            {
                if (ParentRelationship.Children[i] == Entity)
                {
                    for (SIZE_T j = i; j < ParentRelationship.NumChildren - 1; ++j)
                    {
                        ParentRelationship.Children[j] = ParentRelationship.Children[j + 1];
                    }
    
                    --ParentRelationship.NumChildren;
                    break;
                }
            }
        }

        for (size_t i = 0; i < ThisRelationship.NumChildren; ++i)
        {
            entt::entity ChildEntity = ThisRelationship.Children[i];
            FRelationshipComponent& ChildRelationship = Registry.get<FRelationshipComponent>(ChildEntity);
            ChildRelationship.Parent = entt::null;
        }
    }

    void CWorld::OnSineWaveMovementComponentCreated(entt::registry& Registry, entt::entity Entity)
    {
        SSineWaveMovementComponent& MovementComponent = Registry.get<SSineWaveMovementComponent>(Entity);
        MovementComponent.InitialPosition = Registry.get<STransformComponent>(Entity).GetLocation();
    }

    void CWorld::ProcessAnyNewlyLoadedScripts()
    {
        using namespace Scripting;
        
        auto View = EntityRegistry.view<FLuaScriptsContainerComponent>();
        View.each([&](FLuaScriptsContainerComponent& LuaContainerComponent)
        {
            for (uint32 i = 0; i < (uint32)EUpdateStage::Max; ++i)
            {
                LuaContainerComponent.Systems[i].clear();
            }

            FScriptingContext::Get().ForEachScript<FLuaSystemScriptEntry>([&](FLuaSystemScriptEntry& Script)
            {
                LuaContainerComponent.Systems[Script.Stage].emplace_back(Script);
            });
        });
    }

    void CWorld::DrawDebugLine(const glm::vec3& Start, const glm::vec3& End, const glm::vec4& Color, float Thickness, float Duration)
    {
        FLineBatcherComponent& Batcher = GetOrCreateLineBatcher();
        Batcher.DrawLine(Start, End, Color, Thickness, Duration);
    }

    void CWorld::DrawDebugBox(const glm::vec3& Center, const glm::vec3& Extents, const glm::quat& Rotation, const glm::vec4& Color, float Thickness, float Duration)
    {
        FLineBatcherComponent& Batcher = GetOrCreateLineBatcher();
        Batcher.DrawBox(Center, Extents, Rotation, Color, Thickness, Duration);
    }

    void CWorld::DrawDebugSphere(const glm::vec3& Center, float Radius, const glm::vec4& Color, uint8 Segments, float Thickness, float Duration)
    {
        FLineBatcherComponent& Batcher = GetOrCreateLineBatcher();
        Batcher.DrawSphere(Center, Radius, Color, Segments, Thickness, Duration);
    }

    void CWorld::DrawDebugCone(const glm::vec3& Apex, const glm::vec3& Direction, float AngleRadians, float Length, const glm::vec4& Color, uint8 Segments, uint8 Stacks, float Thickness, float Duration)
    {
        FLineBatcherComponent& Batcher = GetOrCreateLineBatcher();
        Batcher.DrawCone(Apex, Direction, AngleRadians, Length, Color, Segments, Stacks, Thickness, Duration);
    }

    void CWorld::DrawFrustum(const glm::mat4& Matrix, float zNear, float zFar, const glm::vec4& Color, float Thickness, float Duration)
    {
        FLineBatcherComponent& Batcher = GetOrCreateLineBatcher();
        Batcher.DrawFrustum(Matrix, zNear, zFar, Color, Thickness, Duration);
    }

    void CWorld::DrawArrow(const glm::vec3& Start, const glm::vec3& Direction, float Length, const glm::vec4& Color, float Thickness, float Duration, float HeadSize)
    {
        FLineBatcherComponent& Batcher = GetOrCreateLineBatcher();
        Batcher.DrawArrow(Start, Direction, Length, Color, Thickness, Duration, HeadSize);
    }

    void CWorld::DrawViewVolume(const FViewVolume& ViewVolume, const glm::vec4& Color, float Thickness, float Duration)
    {
        FLineBatcherComponent& Batcher = GetOrCreateLineBatcher();
        Batcher.DrawViewVolume(ViewVolume, Color, Thickness, Duration);
    }

    TOptional<FRayResult> CWorld::CastRay(const FRayCastSettings& Settings)
    {
        LUMINA_PROFILE_SCOPE();
        
        if (PhysicsScene == nullptr)
        {
            return eastl::nullopt;
        }
        
        TOptional<FRayResult> Result = PhysicsScene->CastRay(Settings.Start,Settings.End, Settings.LayerMask, Settings.IgnoreBodies);
        
        if (Settings.bDrawDebug)
        {
            if (Result.has_value())
            {
                FRayResult RayResult = Result.value();
                DrawDebugLine(Settings.Start, RayResult.Location, FColor(Settings.DebugMissColor));
                
                glm::vec3 NormalEnd = RayResult.Location + RayResult.Normal * 0.5f;
                DrawDebugLine(RayResult.Location, NormalEnd, FColor::Blue);
                
                DrawDebugBox(RayResult.Location, glm::vec3(0.05f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), FColor::Yellow);
                
                DrawDebugLine(RayResult.Location, Settings.End, FColor(Settings.DebugHitColor));
            }
            else
            {
                DrawDebugLine(Settings.Start, Settings.End, FColor(Settings.DebugMissColor));
            }
        }
        
        return Result;
    }

    TOptional<FRayResult> CWorld::CastRay(const glm::vec3& Start, const glm::vec3& End, bool bDrawDebug, uint32 LayerMask, int64 IgnoreBody)
    {
        FRayCastSettings Settings;
        Settings.Start = Start;
        Settings.End = End;
        Settings.LayerMask = LayerMask;
        Settings.bDrawDebug = bDrawDebug;
        Settings.IgnoreBodies.push_back(IgnoreBody);
        
        return CastRay(Settings);
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
