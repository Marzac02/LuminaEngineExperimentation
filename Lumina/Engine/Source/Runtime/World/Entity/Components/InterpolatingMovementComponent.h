#pragma once

#include <glm/glm.hpp>
#include "Core/Math/Sine.h"
#include "InterpolatingMovementComponent.generated.h"

namespace Lumina
{
    
    REFLECT(Component)
    struct LUMINA_API SInterpolatingMovementComponent
    {
        GENERATED_BODY()

        PROPERTY(Editable, Category = "Interp")
        FTransform Start;
        
        PROPERTY(Editable, Category = "Interp")
        FTransform End;

        PROPERTY(Editable, Category = "Interp")
        float Speed = 1.0f;

        FTransform OriginTransform;
        float Alpha = 0.0f;
        bool bForward = true;
    };

    REFLECT(Component)
    struct LUMINA_API SSineWaveMovementComponent
    {
        GENERATED_BODY()
    
        // Wave Parameters
        PROPERTY(Editable, Category = "Wave")
        glm::vec3 Axis = glm::vec3(0.0f, 1.0f, 0.0f);
    
        PROPERTY(Editable, Category = "Wave")
        float Amplitude = 1.0f;
    
        PROPERTY(Editable, Category = "Wave")
        float Frequency = 1.0f;
    
        PROPERTY(Editable, Category = "Wave")
        float PhaseOffset = 0.0f;
    
        // Optional Multi-Axis Movement
        PROPERTY(Editable, Category = "Multi-Axis")
        bool bEnableSecondaryAxis = false;
    
        PROPERTY(Editable, Category = "Multi-Axis")
        glm::vec3 SecondaryAxis = glm::vec3(1.0f, 0.0f, 0.0f);
    
        PROPERTY(Editable, Category = "Multi-Axis")
        float SecondaryAmplitude = 0.5f;
    
        PROPERTY(Editable, Category = "Multi-Axis")
        float SecondaryFrequency = 1.5f;
    
        PROPERTY(Editable, Category = "Multi-Axis")
        float SecondaryPhaseOffset = 0.0f;
    
        // Wave Modifiers
        PROPERTY(Editable, Category = "Modifiers")
        ESineWaveType WaveType = ESineWaveType::Sine;
    
        PROPERTY(Editable, Category = "Modifiers")
        bool bDampingEnabled = false;
    
        PROPERTY(Editable, Category = "Modifiers")
        float DampingFactor = 0.1f;
    
        PROPERTY(Editable, Category = "Modifiers")
        bool bAmplitudeModulation = false;
    
        PROPERTY(Editable, Category = "Modifiers")
        float AmplitudeModulationFrequency = 0.5f;
    
        // Behavior
        PROPERTY(Editable, Category = "Behavior")
        bool bUseRelativeMovement = true;
    
        PROPERTY(Editable, Category = "Behavior")
        bool bAutoStart = true;
    
        PROPERTY(Editable, Category = "Behavior")
        float StartDelay = 0.0f;
    
        glm::vec3 InitialPosition = glm::vec3(0.0f);
        float CurrentTime = 0.0f;
        float CurrentAmplitude = 0.0f;
        bool bInitialized = false;
        bool bIsActive = false;
    };
    
}
