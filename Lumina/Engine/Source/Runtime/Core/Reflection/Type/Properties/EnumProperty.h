#pragma once
#include "Core/Object/Class.h"
#include "Core/Reflection/Type/LuminaTypes.h"
#include "Memory/SmartPtr.h"

namespace Lumina
{
    class FEnumProperty : public FProperty
    {
    public:

        FEnumProperty(const FFieldOwner& InOwner, const FPropertyParams* Params)
            :FProperty(InOwner, Params)
        {
            auto* EnumParams = static_cast<const FEnumPropertyParams*>(Params);
            CEnum* InternalEnum = EnumParams->EnumFunc();
            Assert(InternalEnum)
            SetEnum(InternalEnum);
        }
        
        void AddProperty(FProperty* Property) override { InnerProperty.reset(static_cast<FNumericProperty*>(Property)); }
        LUMINA_API FNumericProperty* GetInnerProperty() const { return InnerProperty.get(); }
        
        void SetEnum(CEnum* InEnum);

        /** Returns the pointer to the internal enum */
        FORCEINLINE CEnum* GetEnum() const { return Enum; }

        void Serialize(FArchive& Ar, void* Value) override;
        void SerializeItem(IStructuredArchive::FSlot Slot, void* Value, void const* Defaults) override;
    
        
    private:

        /** Numeric property which represents the current value of this enum */
        TUniquePtr<FNumericProperty> InnerProperty;

        /** The actual enum class object this property represents */
        CEnum* Enum = nullptr;
    
    };
}
