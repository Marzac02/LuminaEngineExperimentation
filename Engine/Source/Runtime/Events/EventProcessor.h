#pragma once

#include "Containers/Array.h"
#include "Events/Event.h"

namespace Lumina
{
    class IEventHandler
    {
    public:

        /** Called when an event is received, return true to stop propagation and mark as handled. */
        virtual bool OnEvent(FEvent& Event) { return false; }
    };

    class FEventProcessor
    {
    public:

        void RegisterEventHandler(IEventHandler* InHandler);
        void UnregisterEventHandler(IEventHandler* InHandler);

        void Clear();
        

        template<typename TEvent, typename... Args>
        requires(eastl::is_base_of_v<FEvent, TEvent> && eastl::is_constructible_v<TEvent, Args&&...>)
        void Dispatch(Args&&... InArgs)
        {
            TEvent Event(eastl::forward<Args>(InArgs)...);
            DispatchEvent(Event);
        }


    private:

        void DispatchEvent(FEvent& Event);
        
        
    private:

        TVector<IEventHandler*> EventHandlers;
    };
}
