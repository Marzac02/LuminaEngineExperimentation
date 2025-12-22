#pragma once
#include "EdGraphNode.h"
#include "Core/Object/ObjectMacros.h"
#include "ednode_reroute.generated.h"

namespace Lumina
{
    class CMaterialInput;
    class CMaterialOutput;
}

namespace Lumina
{
    REFLECT()
    class CEdNode_Reroute : public CEdGraphNode
    {
        GENERATED_BODY()
    public:

        void BuildNode() override;
        bool WantsTitlebar() const override { return false; }
        FString GetNodeDisplayName() const override { return "Reroute"; }
        FFixedString GetNodeCategory() const override { return "Utility"; }
        ImVec2 GetMinNodeTitleBarSize() const override { return ImVec2(0, 0); }
        
        CMaterialOutput* Output = nullptr;
        CMaterialInput* Input = nullptr;
        
    };
}
