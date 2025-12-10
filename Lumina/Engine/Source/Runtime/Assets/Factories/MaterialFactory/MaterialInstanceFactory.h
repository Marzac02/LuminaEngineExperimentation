#pragma once
#include "Assets/AssetTypes/Material/MaterialInstance.h"
#include "Assets/Factories/Factory.h"
#include "MaterialInstanceFactory.generated.h"


namespace Lumina
{
    REFLECT()
    class CMaterialInstanceFactory : public CFactory
    {
        GENERATED_BODY()
    public:

        CObject* CreateNew(const FName& Name, CPackage* Package) override;
        CClass* GetAssetClass() const override { return CMaterialInstance::StaticClass(); }
        FString GetAssetName() const override { return "Material Instance"; }
        FString GetDefaultAssetCreationName(const FString& InPath) override { return "NewMaterialInstance"; }

        FString GetAssetDescription() const override { return "An instance of a material."; }
        
        bool HasCreationDialogue() const override;
        bool DrawCreationDialogue(const FString& Path, bool& bShouldClose) override;

    private:

        CMaterial* SelectedMaterial = nullptr;
    };
}
