#pragma once

#include "RenderGraphPass.h"
#include "Memory/Allocators/Allocator.h"

namespace Lumina
{
    template <Concept::TExecutor ExecutorType>
    FRGPassHandle FRenderGraph::AddPass(ERGPassFlags PassFlags, FRGEvent&& Event, const FRGPassDescriptor* Parameters, ExecutorType&& Executor)
    {
        FRGPassHandle Pass =  GraphAllocator.TAlloc<TRGPass<ExecutorType>>(std::move(Event), PassFlags, Parameters, std::forward<ExecutorType>(Executor));
        PassGroups.emplace_back().push_back(Pass);

        return Pass;
    }

    template <Concept::TExecutor ExecutorType>
    FRGPassHandle FRenderGraph::AddPassToGroup(TVector<FRGPassHandle>& Group, ERGPassFlags PassFlags, FRGEvent&& Event, const FRGPassDescriptor* Parameters, ExecutorType&& Executor)
    {
        FRGPassHandle Pass =  GraphAllocator.TAlloc<TRGPass<ExecutorType>>(std::move(Event), PassFlags, Parameters, std::forward<ExecutorType>(Executor));
        Group.push_back(Pass);

        return Pass;
    }

    template<typename ... TSpecs>
    void FRenderGraph::AddParallelPasses(TSpecs&&... Specs)
    {
        auto& Group = PassGroups.emplace_back();
        
        (AddPassToGroup(Group, Specs.PassFlags, Move(Specs.Event), Specs.Descriptor, Forward<decltype(Specs.Executor)>(Specs.Executor)), ...);
    }
}