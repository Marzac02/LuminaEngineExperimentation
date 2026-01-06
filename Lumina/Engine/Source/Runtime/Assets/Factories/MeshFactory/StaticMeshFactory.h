#pragma once

#include "Assets/Factories/Factory.h"
#include "Assets/AssetTypes/Mesh/StaticMesh/StaticMesh.h"
#include "Tools/Import/ImportHelpers.h"
#include "StaticMeshFactory.generated.h"

namespace Lumina
{
    REFLECT()
    class CStaticMeshFactory : public CFactory
    {
        GENERATED_BODY()
    public:

        CObject* CreateNew(const FName& Name, CPackage* Package) override;
        FString GetAssetName() const override { return "Static Mesh"; }
        FStringView GetDefaultAssetCreationName() override { return "NewMesh"; }

        FString GetAssetDescription() const override { return "A static mesh."; }
        CClass* GetAssetClass() const override { return CStaticMesh::StaticClass(); }
        bool CanImport() override { return true; }
        bool IsExtensionSupported(FStringView Ext) override { return Ext == ".gltf" || Ext == ".glb" || Ext == ".obj"; }

        bool HasImportDialogue() const override { return true; }
        bool DrawImportDialogue(const FFixedString& RawPath, const FFixedString& DestinationPath, eastl::any& ImportSettings, bool& bShouldClose) override;
        void TryImport(const FFixedString& RawPath, const FFixedString& DestinationPath, const eastl::any& ImportSettings) override;
        
    };
}
