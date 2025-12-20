#include "pch.h"
#include "JoltPhysicsScene.h"

#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>

#include <algorithm>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/RayCast.h>

#include "JoltPhysics.h"
#include "JoltUtils.h"
#include "Core/Profiler/Profile.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"
#include "Renderer/RendererUtils.h"
#include "World/Entity/Components/PhysicsComponent.h"
#include "World/World.h"
#include "World/Entity/Components/CharacterComponent.h"
#include "World/Entity/Components/CharacterControllerComponent.h"
#include "World/Entity/Components/DirtyComponent.h"
#include "World/Entity/Components/TransformComponent.h"
#include "world/entity/components/velocitycomponent.h"

using namespace JPH::literals;

namespace Lumina::Physics
{

    constexpr JPH::EMotionType ToJoltMotionType(EBodyType BodyType)
    {
        switch (BodyType)
        {
            case EBodyType::None:       return JPH::EMotionType::Static;
            case EBodyType::Static:     return JPH::EMotionType::Static;
            case EBodyType::Kinematic:  return JPH::EMotionType::Kinematic;
            case EBodyType::Dynamic:    return JPH::EMotionType::Dynamic;
        }

        LUMINA_NO_ENTRY()
    }

    constexpr JPH::ObjectLayer ToJoltObjectType(EBodyType BodyType)
    {
        if (BodyType == EBodyType::Dynamic || BodyType == EBodyType::Kinematic)
        {
            return Layers::MOVING;
        }

        return Layers::NON_MOVING;
    }
    
    class FObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
    {
    public:
        virtual bool ShouldCollide(JPH::ObjectLayer LayerA, JPH::BroadPhaseLayer LayerB) const override
        {
            switch (LayerA)
            {
                case Layers::NON_MOVING:    return LayerB == BroadPhaseLayers::MOVING;
                case Layers::MOVING:        return true;

                
                default: JPH_ASSERT(false);
                return false;
            }
        }
    };

    class FObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
    {
    public:
        
        bool ShouldCollide(JPH::ObjectLayer ObjectA, JPH::ObjectLayer ObjectB) const override
        {
            switch (ObjectA)
            {
                case Layers::NON_MOVING:    return ObjectB == Layers::MOVING;   // Non-moving only collides with moving
                case Layers::MOVING:        return true;                        // Moving collides with everything

                
                default: JPH_ASSERT(false);
                return false;
            }
        }
    };

    static FObjectLayerPairFilterImpl GObjectVsObjectLayerFilter;
    static FObjectVsBroadPhaseLayerFilterImpl GObjectVsBroadPhaseLayerFilter;


    void JoltContactListener::OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
    {
        
    }

    void JoltContactListener::OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
    {
        ContactListener::OnContactPersisted(inBody1, inBody2, inManifold, ioSettings);
    }

    void JoltContactListener::OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair)
    {
        ContactListener::OnContactRemoved(inSubShapePair);
    }

    void JoltContactListener::OverrideFrictionAndRestitution(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
    {
    }

    void JoltContactListener::GetFrictionAndRestitution(const JPH::Body& inBody, const JPH::SubShapeID& inSubShapeID, float& outFriction, float& outRestitution) const
    {
    }

    FJoltPhysicsScene::FJoltPhysicsScene(CWorld* InWorld)
        : World(InWorld)
    {
        JoltSystem = MakeUniquePtr<JPH::PhysicsSystem>();
        JoltInterfaceLayer = MakeUniquePtr<FLayerInterfaceImpl>();
        
        JoltSystem->Init(65536, 0, 131072, 262144, *JoltInterfaceLayer, GObjectVsBroadPhaseLayerFilter, GObjectVsObjectLayerFilter);
        JoltSystem->SetGravity(JPH::Vec3Arg(0.0f, GEarthGravity, 0.0f));

        JPH::PhysicsSettings JoltSettings;
        JoltSystem->SetPhysicsSettings(JoltSettings);
    }

    void FJoltPhysicsScene::PreUpdate()
    {
        entt::registry& Registry = World->GetEntityRegistry();

        double DeltaTime = GEngine->GetUpdateContext().GetDeltaTime();

        const JPH::BodyLockInterfaceNoLock& LockInterface = JoltSystem->GetBodyLockInterfaceNoLock();
        JPH::BodyInterface& BodyInterface = JoltSystem->GetBodyInterface();
        
        auto BodySyncView = Registry.view<SRigidBodyComponent, STransformComponent, FNeedsTransformUpdate>();
        BodySyncView.each([&](const SRigidBodyComponent& BodyComponent, const STransformComponent& TransformComponent, const FNeedsTransformUpdate& Update)
        {
            JPH::BodyID BodyID = JPH::BodyID(BodyComponent.BodyID);

            JPH::BodyLockRead Lock(LockInterface, BodyID);
            if (Lock.Succeeded())
            {
                const JPH::Body& Body = Lock.GetBody();
                
                JPH::RVec3 Location = JoltUtils::ToJPHRVec3(TransformComponent.GetLocation());
                JPH::Quat Rotation = JoltUtils::ToJPHQuat(TransformComponent.GetRotation());
                JPH::EActivation Activation = Update.bActivate ? JPH::EActivation::Activate : JPH::EActivation::DontActivate;
                
                if (Body.IsStatic())
                {
                    BodyInterface.SetPositionAndRotationWhenChanged(BodyID, Location, Rotation, Activation);
                }
                else if (Body.IsKinematic())
                {
                    BodyInterface.MoveKinematic(BodyID, Location, Rotation, static_cast<float>(DeltaTime));
                }
                else if (Body.IsDynamic())
                {
                    switch (Update.MoveMode)
                    {
                        case EMoveMode::Teleport:
                        {
                            BodyInterface.SetPositionAndRotation(BodyID, Location, Rotation, Activation);
                            break;   
                        }
                        case EMoveMode::MoveKinematic:
                        {
                            JPH::RVec3 CurrentPos = Body.GetPosition();
                            JPH::Quat CurrentRot = Body.GetRotation();
                            
                            JPH::Vec3 LinearVel = (Location - CurrentPos) / static_cast<float>(DeltaTime);
                            BodyInterface.SetLinearVelocity(BodyID, LinearVel);
                            
                            JPH::Quat DeltaRot = Rotation * CurrentRot.Conjugated();
                            JPH::Vec3 Axis;
                            float Angle;
                            DeltaRot.GetAxisAngle(Axis, Angle);
                            
                            if (Angle > JPH::JPH_PI)
                            {
                                Angle -= 2.0f * JPH::JPH_PI;
                            }
                                
                            JPH::Vec3 AngularVel = Axis * (Angle / static_cast<float>(DeltaTime));
                            BodyInterface.SetAngularVelocity(BodyID, AngularVel);
                                
                            if (Update.bActivate)
                            {
                                BodyInterface.ActivateBody(BodyID);
                            }
                            break;
                        }
                        case EMoveMode::ActivateOnly:
                        {
                            BodyInterface.ActivateBody(BodyID);
                            break;
                        }
                    }
                }
            }
        });
        
        auto View = Registry.view<SCharacterControllerComponent, SCharacterPhysicsComponent, SCharacterMovementComponent>();

        View.each([&](SCharacterControllerComponent& Controller, const SCharacterPhysicsComponent& Physics, SCharacterMovementComponent& Movement)
        {
            JPH::CharacterVirtual* Character = Physics.Character.GetPtr();
            if (Character == nullptr)
            {
                return;
            }

            JPH::CharacterVirtual::EGroundState GroundState = Character->GetGroundState();
            bool bWasGrounded = Movement.bGrounded;
            Movement.bGrounded = (GroundState == JPH::CharacterVirtual::EGroundState::OnGround);

            if (!bWasGrounded && Movement.bGrounded)
            {
                Movement.JumpCount = 0;
            }

            glm::vec3 DesiredDirection(0.0f);
            if (glm::length(Controller.MoveInput) > 0.001f)
            {
                glm::vec3 Forward = RenderUtils::GetForwardVector(Controller.LookInput.x, 0.0f);
                glm::vec3 Right = RenderUtils::GetRightVector(Controller.LookInput.x);

                Forward = glm::normalize(glm::vec3(Forward.x, 0.0f, Forward.z));
                Right = glm::normalize(glm::vec3(Right.x, 0.0f, Right.z));

                DesiredDirection = Forward * Controller.MoveInput.y + Right * Controller.MoveInput.x;
                DesiredDirection = glm::normalize(DesiredDirection);
            }

            float TargetSpeed = Movement.MoveSpeed;
            glm::vec3 TargetVelocity = DesiredDirection * TargetSpeed;

            glm::vec3 HorizontalVelocity(Movement.Velocity.x, 0.0f, Movement.Velocity.z);

            float AccelerationRate = Movement.Acceleration;
            float DecelerationRate = Movement.Deceleration;

            if (glm::length(TargetVelocity) > 0.001f)
            {
                HorizontalVelocity = glm::mix(HorizontalVelocity, TargetVelocity, AccelerationRate * (float)DeltaTime);
            }
            else
            {
                float Friction = 1.0f - Movement.GroundFriction * (float)DeltaTime;
                HorizontalVelocity *= glm::max(0.0f, Friction);
            }

            Movement.Velocity.x = HorizontalVelocity.x;
            Movement.Velocity.z = HorizontalVelocity.z;

            if (Movement.bWantsToJump)
            {
                Movement.Velocity.y = Movement.JumpSpeed;
                Movement.bWantsToJump = false;
                Movement.JumpCount++;
            }

            if (Movement.bGrounded)
            {
                JPH::Vec3 GroundVelocity = Character->GetGroundVelocity();
                Movement.Velocity.x += GroundVelocity.GetX();
                Movement.Velocity.z += GroundVelocity.GetZ();
            }
            else
            {
                Movement.Velocity.y += Movement.Gravity * (float)DeltaTime;
            }
            

            Character->SetLinearVelocity(JoltUtils::ToJPHRVec3(Movement.Velocity));

            JPH::CharacterVirtual::ExtendedUpdateSettings UpdateSettings;
            UpdateSettings.mStickToFloorStepDown = JPH::Vec3(0.0f, -0.5f, 0.0f);
            UpdateSettings.mWalkStairsStepUp = JPH::Vec3(0.0f, 0.04f, 0.0f);

            Character->ExtendedUpdate((float)DeltaTime,
                JPH::Vec3(0.0f, Movement.Gravity, 0.0f),
                UpdateSettings,
                JoltSystem->GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
                JoltSystem->GetDefaultLayerFilter(Layers::MOVING),
                {},
                {},
                *FJoltPhysicsContext::GetAllocator());

            JPH::Vec3 ActualVelocity = Character->GetLinearVelocity();
            Movement.Velocity = JoltUtils::FromJPHVec3(ActualVelocity);
        });
    }

    void FJoltPhysicsScene::PostUpdate()
    {

    }

    void FJoltPhysicsScene::Update(double DeltaTime)
    {
        LUMINA_PROFILE_SCOPE();

        constexpr double MaxDeltaTime = 0.25;
        constexpr int MaxSteps = 5;
    
        DeltaTime = std::min(DeltaTime, MaxDeltaTime);
        Accumulator += DeltaTime;

        CollisionSteps = static_cast<int>(Accumulator / FixedTimeStep);
        CollisionSteps = std::min(CollisionSteps, MaxSteps);
        
        if (CollisionSteps > 0)
        {
            PreUpdate();

            JoltSystem->Update(FixedTimeStep, CollisionSteps, FJoltPhysicsContext::GetAllocator(), FJoltPhysicsContext::GetThreadPool());
        
            PostUpdate();
            
            Accumulator -= CollisionSteps * FixedTimeStep;
        
            // Clamp accumulator if we hit max steps
            if (CollisionSteps >= MaxSteps)
            {
                Accumulator = std::min(Accumulator, FixedTimeStep);
            }
        }
        
        SyncTransforms();
    }

    void FJoltPhysicsScene::OnWorldSimulate()
    {
        entt::registry& Registry = World->GetEntityRegistry();
        
        auto View = Registry.view<SRigidBodyComponent>();
        
        View.each([&] (entt::entity EntityID, SRigidBodyComponent&)
        {
            OnRigidBodyComponentConstructed(Registry, EntityID);
        });

        auto CharacterView = Registry.view<SCharacterPhysicsComponent>();
        
        CharacterView.each([&] (entt::entity EntityID, SCharacterPhysicsComponent&)
        {
            OnCharacterComponentConstructed(Registry, EntityID);
        });
        
        JoltSystem->OptimizeBroadPhase();
        
        Registry.on_construct<SCharacterPhysicsComponent>().connect<&FJoltPhysicsScene::OnCharacterComponentConstructed>(this);
        
        Registry.on_construct<SRigidBodyComponent>().connect<&FJoltPhysicsScene::OnRigidBodyComponentConstructed>(this);
        Registry.on_destroy<SRigidBodyComponent>().connect<&FJoltPhysicsScene::OnRigidBodyComponentDestroyed>(this);
    }

    void FJoltPhysicsScene::OnWorldStopSimulate()
    {
        entt::registry& Registry = World->GetEntityRegistry();

        Registry.on_construct<SCharacterPhysicsComponent>().disconnect<&FJoltPhysicsScene::OnCharacterComponentConstructed>(this);
        
        Registry.on_construct<SRigidBodyComponent>().disconnect<&FJoltPhysicsScene::OnRigidBodyComponentConstructed>(this);
        Registry.on_destroy<SRigidBodyComponent>().disconnect<&FJoltPhysicsScene::OnRigidBodyComponentDestroyed>(this);

        auto View = Registry.view<SRigidBodyComponent>();
        View.each([&] (entt::entity EntityID, SRigidBodyComponent&)
        {
           OnRigidBodyComponentDestroyed(Registry, EntityID); 
        });
    }

    void FJoltPhysicsScene::SyncTransforms()
    {
        LUMINA_PROFILE_SCOPE();

        const JPH::BodyLockInterfaceNoLock& LockInterface = JoltSystem->GetBodyLockInterfaceNoLock();
        entt::registry& Registry = World->GetEntityRegistry();

        auto View = Registry.view<SRigidBodyComponent, STransformComponent>();
        View.each([&](entt::entity EntityID, const SRigidBodyComponent& BodyComponent, STransformComponent& TransformComponent)
        {
            const JPH::Body* Body = LockInterface.TryGetBody(JPH::BodyID(BodyComponent.BodyID));
            
            if (Body == nullptr || !Body->IsActive())
            {
                return;
            }
            
            JPH::RVec3 Pos = Body->GetPosition();
            JPH::Quat Rot = Body->GetRotation();
        
            TransformComponent.SetLocation(JoltUtils::FromJPHRVec3(Pos));
            TransformComponent.SetRotation(JoltUtils::FromJPHQuat(Rot));
            
            Registry.emplace_or_replace<FNeedsTransformUpdate>(EntityID);
        });

        auto CharacterView = Registry.view<SCharacterPhysicsComponent, STransformComponent>();
        CharacterView.each([&](entt::entity Entity, const SCharacterPhysicsComponent& CharacterComponent, STransformComponent& TransformComponent)
        {
            JPH::Vec3 Pos = CharacterComponent.Character->GetPosition();
            JPH::Quat Rot = CharacterComponent.Character->GetRotation();
        
            TransformComponent.SetLocation(JoltUtils::FromJPHRVec3(Pos));
            TransformComponent.SetRotation(JoltUtils::FromJPHQuat(Rot));
            
            Registry.emplace_or_replace<FNeedsTransformUpdate>(Entity);   
        });
    }

    TOptional<FRayResult> FJoltPhysicsScene::CastRay(const glm::vec3& Start, const glm::vec3& End, uint32 LayerMask, TSpan<const int64> IgnoreBody)
    {
        JPH::Vec3 JPHStart  = JoltUtils::ToJPHVec3(Start);
        JPH::Vec3 JPHEnd    = JoltUtils::ToJPHVec3(End);
        JPH::Vec3 Direction = JPHEnd - JPHStart;
        
        if (Direction.Length() < LE_SMALL_NUMBER)
        {
            return eastl::nullopt;
        }
        
        JPH::RRayCast Ray;
        Ray.mOrigin = JPHStart;
        Ray.mDirection = Direction;
        
        class IgnoreFilter : public JPH::BodyFilter
        {
        public:
            IgnoreFilter(TSpan<const int64> InIgnoreBodies)
            {
                eastl::transform(
                         InIgnoreBodies.begin(), 
                         InIgnoreBodies.end(),
                         eastl::insert_iterator(IgnoreBodies, IgnoreBodies.end()),
                         [](const int64& Body) { return Body; }
                     );
            }
        
            bool ShouldCollide(const JPH::BodyID& inBodyID) const override
            {
                return IgnoreBodies.find(inBodyID.GetIndexAndSequenceNumber()) == IgnoreBodies.end();
            }

            TFixedHashSet<int64, 4> IgnoreBodies;
        };
        
        IgnoreFilter IgnoreFilter{IgnoreBody};
        
        
        JPH::RayCastResult Hit;
        bool bHit = JoltSystem->GetNarrowPhaseQuery().CastRay(Ray, Hit, {}, {}, IgnoreFilter);
        if (!bHit)
        {
            return eastl::nullopt;
        }
        
        const JPH::BodyLockInterfaceNoLock& LockInterface = JoltSystem->GetBodyLockInterfaceNoLock();
        
        JPH::Body* Body = LockInterface.TryGetBody(Hit.mBodyID);
        if (!Body)
        {
            return eastl::nullopt;
        }
        
        JPH::Vec3 SurfaceNormal = Body->GetWorldSpaceSurfaceNormal(Hit.mSubShapeID2, Ray.GetPointOnRay(Hit.mFraction));

        
        FRayResult Result;
        Result.BodyID = Hit.mBodyID.GetIndexAndSequenceNumber();
        Result.Entity = static_cast<uint32>(Body->GetUserData());
        Result.Start = Start;
        Result.End = End;
        Result.Normal = glm::normalize(JoltUtils::FromJPHVec3(SurfaceNormal));
        Result.Location = JoltUtils::FromJPHRVec3(Ray.GetPointOnRay(Hit.mFraction));
        Result.Fraction = Hit.mFraction;
        
        return Result;
    }

    void FJoltPhysicsScene::OnCharacterComponentConstructed(entt::registry& Registry, entt::entity Entity)
    {
        SCharacterPhysicsComponent& CharacterComponent = Registry.get<SCharacterPhysicsComponent>(Entity);
        STransformComponent& TransformComponent = Registry.get<STransformComponent>(Entity);

        JPH::Ref<JPH::CharacterVirtualSettings> Settings = Memory::New<JPH::CharacterVirtualSettings>();
        
        
        JPH::Ref<JPH::Shape> StandingShape = JPH::RotatedTranslatedShapeSettings(
            JPH::Vec3(0, CharacterComponent.HalfHeight, 0),
            JPH::Quat::sIdentity(),
            Memory::New<JPH::CapsuleShape>(CharacterComponent.HalfHeight, CharacterComponent.Radius * TransformComponent.MaxScale())).Create().Get();

        Settings->mShape = StandingShape;
        Settings->mMass = CharacterComponent.Mass;
        Settings->mMaxStrength = CharacterComponent.MaxStrength;
        Settings->mCharacterPadding = 0.02f;
        Settings->mPenetrationRecoverySpeed = 1.0f;
        Settings->mPredictiveContactDistance = 0.1f;

        Settings->mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), 0.0f);

        JPH::CharacterVirtual* Character = Memory::New<JPH::CharacterVirtual>(Settings,
            JoltUtils::ToJPHRVec3(TransformComponent.GetLocation()),
            JoltUtils::ToJPHQuat(TransformComponent.GetRotation()),
            0,
            JoltSystem.get());
        
        CharacterComponent.Character = Character;
    }

    void FJoltPhysicsScene::OnRigidBodyComponentConstructed(entt::registry& Registry, entt::entity EntityID)
    {
        if (!Registry.any_of<SSphereColliderComponent, SBoxColliderComponent>(EntityID))
        {
            LOG_ERROR("Entity {} attempted to construct a rigid body without a collider!");
            return;
        }

        SRigidBodyComponent& RigidBodyComponent = Registry.get<SRigidBodyComponent>(EntityID);
        STransformComponent& TransformComponent = Registry.get<STransformComponent>(EntityID);
        
        JPH::ShapeRefC Shape;

        if (Registry.all_of<SBoxColliderComponent>(EntityID))
        {
            SBoxColliderComponent& BC = Registry.get<SBoxColliderComponent>(EntityID);
            JPH::BoxShapeSettings Settings(JoltUtils::ToJPHVec3(BC.HalfExtent * TransformComponent.GetScale()));
            Settings.SetEmbedded();
            Shape = Settings.Create().Get();
        }
        else if (Registry.all_of<SSphereColliderComponent>(EntityID))
        {
            SSphereColliderComponent& SC = Registry.get<SSphereColliderComponent>(EntityID);
            JPH::SphereShapeSettings Settings(SC.Radius * TransformComponent.MaxScale());
            Settings.SetEmbedded();
            Shape = Settings.Create().Get();
        }

        JPH::ObjectLayer Layer      = ToJoltObjectType(RigidBodyComponent.BodyType);
        JPH::EMotionType MotionType = ToJoltMotionType(RigidBodyComponent.BodyType);

        glm::quat Rotation      = TransformComponent.GetRotation();
        glm::vec3 Position      = TransformComponent.GetLocation();

        JPH::BodyCreationSettings Settings(
            Shape,
            JoltUtils::ToJPHRVec3(Position),
            JoltUtils::ToJPHQuat(Rotation),
            MotionType,
            Layer);

        Settings.mRestitution = 0.5f;
        Settings.mFriction = 0.3f;
        Settings.mAngularDamping = RigidBodyComponent.AngularDamping;
        Settings.mLinearDamping = RigidBodyComponent.LinearDamping;

        JPH::BodyInterface& BodyInterface = JoltSystem->GetBodyInterface();
        
        JPH::Body* Body = BodyInterface.CreateBody(Settings);
        Body->SetUserData(static_cast<uint64>(EntityID));
        BodyInterface.AddBody(Body->GetID(), JPH::EActivation::Activate);
        RigidBodyComponent.BodyID = Body->GetID().GetIndexAndSequenceNumber();
    }

    void FJoltPhysicsScene::OnRigidBodyComponentDestroyed(entt::registry& Registry, entt::entity EntityID)
    {
        SRigidBodyComponent& RigidBodyComponent = Registry.get<SRigidBodyComponent>(EntityID);
        JPH::BodyInterface& BodyInterface = JoltSystem->GetBodyInterface();
        JPH::BodyID BodyID(RigidBodyComponent.BodyID);
        
        BodyInterface.RemoveBody(BodyID);
        BodyInterface.DestroyBody(BodyID);
    }

    void FJoltPhysicsScene::OnColliderComponentAdded(entt::registry& Registry, entt::entity EntityID)
    {
    }

    void FJoltPhysicsScene::OnColliderComponentRemoved(entt::registry& Registry, entt::entity EntityID)
    {
    }
}
