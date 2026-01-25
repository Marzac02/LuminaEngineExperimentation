#pragma once
#include "ObjectCore.h"
#include "Containers/Array.h"

namespace Lumina
{
    
    template<typename T>
    class TDeferredRegistry
    {
    public:
        
        using TType = T::TType;

        static TDeferredRegistry& Get()
        {
            static TDeferredRegistry Registry;
            return Registry;
        }

        struct FRegistrant
        {
            TType* (*RegisterFunc)();
        };


        void AddRegistration(TType* (*RegisterFunc)())
        {
            Registrations.emplace_back(RegisterFunc);
        }

        template<typename FuncType>
        void ProcessRegistrations(FuncType&& OnRegistration)
        {
            size_t Num = Registrations.size();
            for (size_t Index = ProcessedRegistrations; Index < Num; ++Index)
            {
                TType* Object = Registrations[Index].RegisterFunc();
                OnRegistration(*Object);
            }
            
            ProcessedRegistrations = Num;
        }
        
        void ProcessRegistrations()
        {
            size_t Num = Registrations.size();
            for (size_t Index = ProcessedRegistrations; Index < Num; ++Index)
            {
                Registrations[Index].RegisterFunc();
            }
            
            ProcessedRegistrations = Num;
        }

        bool HasPendingRegistrations()
        {
            return Registrations.size() != ProcessedRegistrations;
        }

    private:
        
        TFixedVector<FRegistrant, 2024>     Registrations;
        size_t                              ProcessedRegistrations = 0;
        
    };

    using FClassDeferredRegistry = TDeferredRegistry<FClassRegistrationInfo>;
    using FEnumDeferredRegistry = TDeferredRegistry<FEnumRegistrationInfo>;
    using FStructDeferredRegistry = TDeferredRegistry<FStructRegistrationInfo>;
    
}
