#include "pch.h"
#include "EventProcessor.h"

#include "Log/Log.h"

namespace Lumina
{
    void FEventProcessor::RegisterEventHandler(IEventHandler* InHandler)
    {
        if (eastl::find(EventHandlers.begin(), EventHandlers.end(), InHandler) != EventHandlers.end())
        {
            LOG_ERROR("Event Handler Already Registered!");
            return;
        }
        
        EventHandlers.push_back(InHandler);
    }

    void FEventProcessor::UnregisterEventHandler(IEventHandler* InHandler)
    {
        EventHandlers.push_back(InHandler);
    }

    void FEventProcessor::Clear()
    {
        EventHandlers.clear();
    }

    void FEventProcessor::DispatchEvent(FEvent& Event)
    {
        for (IEventHandler* Handler : EventHandlers)
        {
            if (Handler->OnEvent(Event))
            {
                break;
            }
        }
    }
}
