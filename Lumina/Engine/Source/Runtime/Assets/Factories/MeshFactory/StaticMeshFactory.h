#pragma once

#include "meshoptimizer.h"
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
        FString GetDefaultAssetCreationName(const FString& InPath) override { return "NewMesh"; }

        FString GetAssetDescription() const override { return "A static mesh."; }
        CClass* GetAssetClass() const override { return CStaticMesh::StaticClass(); }
        bool CanImport() override { return true; }
        bool IsExtensionSupported(const FString& Ext) override { return Ext == ".gltf" || Ext == ".glb" || Ext == ".obj"; }

        bool HasImportDialogue() const override { return true; }
        bool DrawImportDialogue(const FString& RawPath, const FString& DestinationPath, bool& bShouldClose) override;
        void TryImport(const FString& RawPath, const FString& DestinationPath) override;

    private:

        
        Import::Mesh::FMeshImportData       ImportedData;
        Import::Mesh::FMeshImportOptions    Options;
        bool bShouldReimport = true;
    };
}
