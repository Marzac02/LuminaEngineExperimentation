#pragma once

#include "Memory/Memory.h"
#include "Platform/GenericPlatform.h"

namespace Lumina
{
    enum class EUpdateStage : uint8
    {
        FrameStart,
        PrePhysics,
        DuringPhysics,
        PostPhysics,
        FrameEnd,
        Paused,
        Max,
    };
    
    constexpr const char* GUpdateStageNames[] = 
    {
        "FrameStart",
        "PrePhysics",
        "DuringPhysics",
        "PostPhysics",
        "FrameEnd",
        "Paused"
    };

    #define US_FrameStart       EUpdateStage::FrameStart
    #define US_PrePhysics       EUpdateStage::PrePhysics
    #define US_DuringPhysics    EUpdateStage::DuringPhysics
    #define US_PostPhysics      EUpdateStage::PostPhysics
    #define US_FrameEnd         EUpdateStage::FrameEnd
    #define US_Paused           EUpdateStage::Paused

    enum class EUpdatePriority : uint8
    {
        Highest     = 0,
        High        = 64,
        Medium      = 128,
        Low         = 192,
        Disabled    = 255,
        Default     = Medium,
    };

    struct FUpdateStagePriority
    {
        FUpdateStagePriority(EUpdateStage InStage) : Stage(InStage) { }
        FUpdateStagePriority(EUpdateStage InStage, EUpdatePriority InPriority) : Stage(InStage), Priority(InPriority) { }

    public:

        EUpdateStage     Stage;
        EUpdatePriority  Priority = EUpdatePriority::Default;
    };

    using RequiresUpdate = FUpdateStagePriority;
    
    struct FUpdatePriorityList
    {
        FUpdatePriorityList()
        {
            Reset();
        }

        template<typename... Args>
        FUpdatePriorityList(Args&&... args)
        {
            Reset();
            ((*this << static_cast<Args&&>(args)), ...);
        }

        void Reset()
        {
            Memory::Memset(Priorities, (uint8) EUpdatePriority::Disabled, sizeof(Priorities));
        }

        bool IsStageEnabled(EUpdateStage stage) const
        {
            return Priorities[(uint8) stage] != (uint8) EUpdatePriority::Disabled;
        }

        uint8 GetPriorityForStage(EUpdateStage stage) const
        {
            return Priorities[(uint8) stage];
        }

        FUpdatePriorityList& SetStagePriority(FUpdateStagePriority&& stagePriority)
        {
            Priorities[(uint8) stagePriority.Stage] = (uint8)stagePriority.Priority;
            return *this;
        }

        // Set a priority for a given stage
        FUpdatePriorityList& operator<<(FUpdateStagePriority&& stagePriority)
        {
            Priorities[(uint8)stagePriority.Stage] = (uint8)stagePriority.Priority;
            return *this;
        }

        bool AreAllStagesDisabled() const
        {
            const static uint8 DisabledStages[(uint8)EUpdateStage::Max] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
            static_assert(sizeof(DisabledStages) == sizeof(Priorities), "disabled stages must be the same size as the priorities list");
            return memcmp(Priorities, DisabledStages, sizeof(Priorities)) == 0;
        }

    private:

        uint8           Priorities[(uint8)EUpdateStage::Max];
    };
}
