#include "pch.h"
#include "LuminaTypes.h"
#include "Core/Object/Field.h"
#include "Core/Object/Class.h"

namespace Lumina
{
    void FProperty::Init()
    {
        eastl::visit([this](auto& Value)
        {
            Value->AddProperty(this);
        }, Owner);
   
    }

    FString FProperty::GetTypeAsString() const
    {
        return TypeName.ToString();
    }

    FName FProperty::GetTypeAsFName() const
    {
        return TypeName;
    }
    
    void FProperty::OnMetadataFinalized()
    {
        if (Metadata.HasMetadata("DisplayName"))
        {
            DisplayName = Metadata.GetMetadata("DisplayName").ToString();
        }
        else
        {
            DisplayName = MakeDisplayNameFromName(TypeFlags, Name);
        }
    }

    FString FProperty::MakeDisplayNameFromName(EPropertyTypeFlags TypeFlags, const FName& InName)
    {
        FString Raw = InName.ToString();

        if (TypeFlags == EPropertyTypeFlags::Bool)
        {
            if (StringUtils::StartsWith(Raw, "b") && isupper(Raw[1]))
            {
                Raw.erase(0, 1);
            }
        }

        FString Display;
        for (size_t i = 0; i < Raw.size(); ++i)
        {
            if (i > 0 && isupper(Raw[i]) && !isspace(Raw[i - 1]) && !isupper(Raw[i - 1]))
            {
                Display += ' ';
            }
            Display += Raw[i];
        }

        if (!Display.empty())
        {
            Display[0] = toupper(Display[0]);
        }

        return Display;
    }

    void* FProperty::GetValuePtrInternal(void* ContainerPtr, int64 ArrayIndex) const
    {
        void* PropertyPtr = (uint8*)ContainerPtr + Offset;
        return (uint8*)PropertyPtr + ArrayIndex * ElementSize;
    }
}
