#pragma once
#include "Containers/Array.h"
#include "Containers/Name.h"
#include "Core/Threading/Thread.h"
#include "Renderer/RenderResource.h"


namespace Lumina
{
    struct FRenderPassDesc;
    class IVulkanShader;
    struct FComputePipelineDesc;
    struct FGraphicsPipelineDesc;
    class FVulkanDevice;
}


namespace Lumina
{

    class FVulkanPipelineCache
    {
    public:

        struct FShaderPipelineTracker
        {
            TVector<FName> Shaders;
        };

        FRHIGraphicsPipeline* GetOrCreateGraphicsPipeline(FVulkanDevice* Device, const FGraphicsPipelineDesc& InDesc, const FRenderPassDesc& RenderPassDesc);
        FRHIComputePipeline* GetOrCreateComputePipeline(FVulkanDevice* Device, const FComputePipelineDesc& InDesc);

        void PostShaderRecompiled(const FRHIShader* Shader);
        void ReleasePipelines();
        
    private:

        FMutex ShaderMutex;
        THashMap<size_t, FRHIGraphicsPipelineRef>   GraphicsPipelines;
        THashMap<size_t, FRHIComputePipelineRef>    ComputePipelines;
        
    };
}
