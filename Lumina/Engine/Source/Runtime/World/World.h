#pragma once

#include "Core/Object/Object.h"
#include "Core/UpdateContext.h"
#include "World/Entity/Components/CameraComponent.h"
#include "Core/Object/ObjectHandleTyped.h"
#include "Entity/Registry/EntityRegistry.h"
#include "Renderer/RenderGraph/RenderGraph.h"
#include "Memory/SmartPtr.h"
#include "Physics/PhysicsScene.h"
#include "Entity/Systems/SystemContext.h"
#include "Scene/RenderScene/RenderScene.h"
#include "Subsystems/FCameraManager.h"
#include "Physics/Ray/RayCast.h"
#include "Renderer/PrimitiveDrawInterface.h"
#include "Core/Delegates/Delegate.h"
#include "World.generated.h"


namespace Lumina
{
    struct FLineBatcherComponent;
    class CEntitySystem;
}


namespace Lumina
{
    REFLECT()
    class LUMINA_API CWorld : public CObject, public IPrimitiveDrawInterface
    {
        GENERATED_BODY()
        
        friend class FWorldManager;
        friend struct FSystemContext;
        friend struct SRenderComponent;
        
    public:

        CWorld();

        //~ Begin CObject Interface
        void Serialize(FArchive& Ar) override;
        void PreLoad() override;
        void PostLoad() override;
        //~ End CObject Interface
        
        /**
         * Initializes systems and renderer. Must be called before anything is done with the world.
         */
        void InitializeWorld();
        

        /**
         * Called on every update stage and runs systems attached to this world.
         */
        void Update(const FUpdateContext& Context);
        void Paused(const FUpdateContext& Context);
        void Render(FRenderGraph& RenderGraph);

        void UpdateScripts();
        
        /**
         * Called to shut down the world, destroys system, components, and entities.
         */
        void ShutdownWorld();

        bool RegisterSystem(CEntitySystem* NewSystem);

        entt::entity ConstructEntity(const FName& Name, const FTransform& Transform = FTransform());
        
        void CopyEntity(entt::entity& To, entt::entity From);
        void DestroyEntity(entt::entity Entity);

        //LUM_DEPRECATED("0.0.1", "Access to the registry has been deprecated")
        FEntityRegistry& GetEntityRegistry() { return EntityRegistry; }

        //LUM_DEPRECATED("0.0.1", "Access to the registry has been deprecated")
        const FEntityRegistry& GetEntityRegistry_Immutable() const { return EntityRegistry; }

        uint32 GetNumEntities() const;
        void SetActiveCamera(entt::entity InEntity);
        SCameraComponent* GetActiveCamera();
        entt::entity GetActiveCameraEntity() const;
        
        FORCEINLINE entt::entity GetSingletonEntity() const { return SingletonEntity; }

        void OnChangeCameraEvent(const FSwitchActiveCameraEvent& Event);
        
        double GetWorldDeltaTime() const { return DeltaTime; }
        double GetTimeSinceWorldCreation() const { return TimeSinceCreation; }

        void BeginPlay();
        void EndPlay();

        void SetPaused(bool bNewPause) { bPaused = bNewPause; }
        bool IsPaused() const { return bPaused; }

        void SetActive(bool bNewActive) { bActive = bNewActive; }
        bool IsSuspended() const { return !bActive; }

        void SetSimulating(bool bSim)
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
        
        bool IsSimulating() const { return bSimulating; }

        static CWorld* DuplicateWorld(CWorld* OwningWorld);

        IRenderScene* GetRenderer() const { return RenderScene.get(); }

        const TVector<CEntitySystem*>& GetSystemsForUpdateStage(EUpdateStage Stage);

        void OnRelationshipComponentDestroyed(entt::registry& Registry, entt::entity Entity);
        void OnSineWaveMovementComponentCreated(entt::registry& Registry, entt::entity Entity);

        void ProcessAnyNewlyLoadedScripts();

        //~ Begin Debug Drawing
        void DrawLine(const glm::vec3& Start, const glm::vec3& End, const glm::vec4& Color, float Thickness = 1.0f, bool bDepthTest = true, float Duration = 0.0f) override;
        //~ End Debug Drawing
        
        TOptional<FRayResult> CastRay(const FRayCastSettings& Settings);
        TOptional<FRayResult> CastRay(const glm::vec3& Start, const glm::vec3& End, bool bDrawDebug = false, float DebugDuration = 0.0f, uint32 LayerMask = 0xFFFFFFFF, int64 IgnoreBody = -1);
        TVector<FRayResult> CastSphere(const FSphereCastSettings& Settings);

        void SetIsPlayWorld(bool bValue) { bIsPlayWorld = bValue; }
        FORCEINLINE bool IsPlayWorld() const { return bIsPlayWorld; }

        void MarkTransformDirty(entt::entity Entity);
        void SetEntityTransform(entt::entity Entity, const FTransform& NewTransform);

        void SetSelectedEntity(entt::entity EntityID);
        entt::entity GetSelectedEntity() const;

        template<typename TFunc>
        void ForEachUniqueSystem(TFunc&& Func)
        {
            TSet<CEntitySystem*> UniqueSystems;
            for (uint8 i = 0; i < (uint8)EUpdateStage::Max; i++)
            {
                for (CEntitySystem* System : SystemUpdateList[i])
                {
                    if (UniqueSystems.count(System) == 0)
                    {
                        Func(System);
                        UniqueSystems.emplace(System);
                    }
                }
            }
        }
        
    private:
        
        FLineBatcherComponent& GetOrCreateLineBatcher();
    
    private:
        
        FEntityRegistry                                 EntityRegistry;
        entt::dispatcher                                SingletonDispatcher;
        entt::entity                                    SingletonEntity;

        FSystemContext                                  SystemContext;
        FDelegateHandle                                 ScriptUpdatedDelegateHandle;
        
        TUniquePtr<FCameraManager>                      CameraManager;
        TUniquePtr<IRenderScene>                        RenderScene;
        TUniquePtr<Physics::IPhysicsScene>              PhysicsScene;
        
        TVector<CEntitySystem*>                         SystemUpdateList[(int32)EUpdateStage::Max];
        
        
        int64                                           WorldIndex = -1;
        double                                          DeltaTime = 0.0;
        double                                          TimeSinceCreation = 0.0;

        uint32                                          bPaused:1 = true;
        uint32                                          bSimulating:1 = false;
        uint32                                          bActive:1 = true;
        uint32                                          bIsPlayWorld:1 = false;
    };
}
