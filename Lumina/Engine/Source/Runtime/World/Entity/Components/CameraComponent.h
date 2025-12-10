#pragma once
#include "Component.h"
#include "Core/Engine/Engine.h"
#include "Scripting/Lua/Scripting.h"
#include "Renderer/ViewVolume.h"
#include "Core/Object/Class.h"
#include "World/Entity/Registry/EntityRegistry.h"
#include "EntityComponentRegistry.h"
#include "CameraComponent.generated.h"


namespace Lumina
{
    REFLECT()
    struct LUMINA_API SCameraComponent
    {
        GENERATED_BODY()
        ENTITY_COMPONENT(SCameraComponent)
        
        SCameraComponent(float fov = 90.0f, float aspect = 16.0f / 9.0f)
            :ViewVolume(fov, aspect)
        {}

        void SetView(const glm::vec3& Position, const glm::vec3& ViewDirection, const glm::vec3& UpDirection)
        {
            ViewVolume.SetView(Position, ViewDirection, UpDirection);
        }
        
        void SetFOV(float NewFOV)
        {
            ViewVolume.SetFOV(NewFOV);
        }
        
        void SetAspectRatio(float NewAspect)
        {
            ViewVolume.SetPerspective(ViewVolume.GetFOV(), NewAspect);
        }

        void SetPosition(const glm::vec3& NewPosition)
        {
            ViewVolume.SetViewPosition(NewPosition);
        }

        float GetFOV() const { return ViewVolume.GetFOV(); }
        float GetAspectRatio() const { return ViewVolume.GetAspectRatio(); }
        const glm::vec3& GetPosition() const { return ViewVolume.GetViewPosition(); }
        const glm::mat4& GetViewMatrix() const { return ViewVolume.GetViewMatrix(); }
        const glm::mat4& GetProjectionMatrix() const { return ViewVolume.GetProjectionMatrix(); }
        const glm::mat4& GetViewProjectionMatrix() const { return ViewVolume.GetViewProjectionMatrix(); }
        const FViewVolume& GetViewVolume() const { return ViewVolume; }
        const glm::vec3& GetForwardVector() const { return ViewVolume.GetForwardVector(); }
        const glm::vec3& GetRightVector() const { return ViewVolume.GetRightVector(); }

        PROPERTY(Editable, Category = "Camera")
        float FOV = 0.0f;

        PROPERTY(Editable, Category = "Camera")
        bool bAutoActivate = false;
        
    private:
        
        FViewVolume ViewVolume;
    };


    REFLECT()
    struct LUMINA_API SCameraControllerComponent
    {
        GENERATED_BODY()
        ENTITY_COMPONENT(SCameraControllerComponent)
        
        PROPERTY(Editable, Category = "Input")
        float MouseSensitivity = 0.1f;
        
        PROPERTY(Editable, Category = "Input")
        float MinPitch = -89.0f;
        
        PROPERTY(Editable, Category = "Input")
        float MaxPitch = 89.0f;
        
        PROPERTY(Editable, Category = "Input")
        bool bInvertY = false;
        
        float Yaw = 0.0f;
        float Pitch = 0.0f;
        
        entt::entity TargetEntity = entt::null;
    };
    
    REFLECT()
    struct LUMINA_API SFirstPersonCameraComponent
    {
        GENERATED_BODY()
        ENTITY_COMPONENT(SFirstPersonCameraComponent)
        
        PROPERTY(Editable, Category = "First Person")
        float EyeHeight = 1.7f;
    };
    
    REFLECT()
    struct LUMINA_API SThirdPersonCameraComponent
    {
        GENERATED_BODY()
        ENTITY_COMPONENT(SThirdPersonCameraComponent)
        
        PROPERTY(Editable, Category = "Third Person")
        float Distance = 5.0f;
        
        PROPERTY(Editable, Category = "Third Person")
        float MinDistance = 1.0f;
        
        PROPERTY(Editable, Category = "Third Person")
        float MaxDistance = 10.0f;
        
        PROPERTY(Editable, Category = "Third Person")
        glm::vec3 Offset = glm::vec3(0.0f, 2.0f, 0.0f); // Shoulder offset
        
        PROPERTY(Editable, Category = "Third Person")
        float CollisionRadius = 0.3f; // For camera collision
        
        PROPERTY(Editable, Category = "Third Person")
        float LerpSpeed = 10.0f; // Smooth follow
    };
    
    REFLECT()
    struct LUMINA_API SOrbitalCameraComponent
    {
        GENERATED_BODY()
        ENTITY_COMPONENT(SOrbitalCameraComponent)
        
        PROPERTY(Editable, Category = "Orbital")
        float Distance = 10.0f;
        
        PROPERTY(Editable, Category = "Orbital")
        float MinDistance = 2.0f;
        
        PROPERTY(Editable, Category = "Orbital")
        float MaxDistance = 50.0f;
        
        PROPERTY(Editable, Category = "Orbital")
        float ZoomSpeed = 1.0f;
        
        PROPERTY(Editable, Category = "Orbital")
        glm::vec3 FocusPoint;
    };

    struct LUMINA_API FSwitchActiveCameraEvent
    {
        entt::entity NewActiveEntity;
    };


    
}
