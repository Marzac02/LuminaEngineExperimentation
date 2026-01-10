#include "pch.h"
#include "Object.h"
#include "Class.h"
#include "Core/Reflection/Type/LuminaTypes.h"
#include "Package/Package.h"

/** Low level CObject registration. */
extern Lumina::FClassRegistrationInfo Registration_Info_CClass_Lumina_CObject;
    
Lumina::CClass* Construct_CClass_Lumina_CObject()
{
    if (!Registration_Info_CClass_Lumina_CObject.OuterSingleton)
    {
        Registration_Info_CClass_Lumina_CObject.OuterSingleton = Lumina::CObject::StaticClass();
        Lumina::CObjectForceRegistration(Registration_Info_CClass_Lumina_CObject.OuterSingleton);
    }
    ASSERT(Registration_Info_CClass_Lumina_CObject.OuterSingleton->GetClass() != nullptr);
    return Registration_Info_CClass_Lumina_CObject.OuterSingleton;
}
    
IMPLEMENT_CLASS(Lumina, CObject)

namespace Lumina
{
    void CObject::Serialize(FArchive& Ar)
    {
        if (CClass* Class = GetClass())
        {
            Class->SerializeTaggedProperties(Ar, this);
        }
    }

    void CObject::Serialize(IStructuredArchive::FRecord Record)
    {
        if (CClass* Class = GetClass())
        {
            Class->SerializeTaggedProperties(Record.EnterField("Properties").GetArchiver(), this);
        }
    }

    void CObject::PostInitProperties()
    {
        
    }

    bool CObject::Rename(const FName& NewName, CPackage* NewPackage)
    {
		if (NewName == GetName() && NewPackage == GetPackage())
        {
            return true;
        }
        
        HandleNameChange(NewName, NewPackage);
        return true;
    }
}
