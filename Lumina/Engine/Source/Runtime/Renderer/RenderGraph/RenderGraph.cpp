#include "pch.h"
#include "RenderGraph.h"
#include "RenderGraphDescriptor.h"
#include "RenderGraphPass.h"
#include "Core/Engine/Engine.h"
#include "Platform/Process/PlatformProcess.h"
#include "Renderer/RenderContext.h"
#include "Renderer/RHIGlobals.h"
#include "TaskSystem/TaskSystem.h"


namespace Lumina
{
    FRenderGraph::FRenderGraph()
        : GraphAllocator(1024llu * 10llu)
    {
        PassGroups.reserve(24);
    }

    FRGPassDescriptor* FRenderGraph::AllocDescriptor()
    {
        return GraphAllocator.TAlloc<FRGPassDescriptor>();
    }

    void FRenderGraph::Execute()
    {
        LUMINA_PROFILE_SCOPE();
        
        TFixedVector<FLambdaTask, 1> AsyncTasks;
        TFixedVector<ICommandList*, 1> AllCommandLists;
        
		FMutex CommandListMutex;

        FRHICommandListRef CommandList = GRenderContext->CreateCommandList(FCommandListInfo::Graphics());
        AllCommandLists.push_back(CommandList);
        CommandList->Open();
        
        for (size_t PassIndex = 0; PassIndex < PassGroups.size(); ++PassIndex)
        {
            TVector<FRGPassHandle>& Passes = PassGroups[PassIndex];
            for (int i = 0; i < Passes.size(); ++i)
            {
                FRGPassHandle Pass = Passes[i];
                
                // The user has promised us this pass can now run at any time without issues, so we dispatch it and keep going.
                if (Pass->GetDescriptor()->HasAnyFlag(ERGExecutionFlags::Async))
                {
                    Task::AsyncTask(1, 1, [&AllCommandLists, &CommandListMutex, Pass](uint32, uint32, uint32)
                    {
                        FRHICommandListRef LocalCommandList = GRenderContext->CreateCommandList(FCommandListInfo::Graphics());
                    
                        {
                            FScopeLock Lock(CommandListMutex);
                            AllCommandLists.push_back(LocalCommandList);
                        }
            
                        LocalCommandList->Open();
                        Pass->Execute(*LocalCommandList);
                        LocalCommandList->Close();
                    });
                }
                else // Run the pass serially.
                {
                    Pass->Execute(*CommandList);
                }
            }
        }

        CommandList->Close();
        
        GTaskSystem->WaitForAll();
        
        GRenderContext->ExecuteCommandLists(AllCommandLists.data(), (uint32)AllCommandLists.size(), ECommandQueue::Graphics);   
    }
}
