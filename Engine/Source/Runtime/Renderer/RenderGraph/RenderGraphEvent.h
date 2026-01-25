#pragma once

#include "Containers/String.h"

namespace Lumina
{
    class RUNTIME_API FRGEvent
    {
    public:
        FRGEvent() = default;

        explicit FRGEvent(const char* InEventName)
            : StaticName(InEventName)
        {}

        template<typename... Args>
        requires (sizeof ... (Args) > 0)
        FRGEvent(FStringView FormatString, Args&&... FormatArgs)
        {
            FormattedName = std::format(FormatString, std::forward<Args>(FormatArgs)...);
        }

        const char* Get() const
        {
            return StaticName;
        }

        bool IsValid() const
        {
            return StaticName != nullptr || !FormattedName.empty();
        }

    private:
        
        const char* StaticName = nullptr;
        FString FormattedName;
    };
}
