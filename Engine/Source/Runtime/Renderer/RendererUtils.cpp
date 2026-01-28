#include "pch.h"
#include "RendererUtils.h"

#include "RenderContext.h"
#include "RenderResource.h"
#include "RHIGlobals.h"

namespace Lumina::RenderUtils
{
    bool ResizeBufferIfNeeded(FRHIBufferRef& Buffer, uint32 DesiredSize, int GrowthFactor)
    {
        if (Buffer->GetSize() < DesiredSize)
        {
            FRHIBufferDesc Desc = Buffer->GetDescription();
            Desc.Size = DesiredSize * GrowthFactor;
            Buffer = GRenderContext->CreateBuffer(Desc);
            return true;
        }
        
        return false;
    }
}
