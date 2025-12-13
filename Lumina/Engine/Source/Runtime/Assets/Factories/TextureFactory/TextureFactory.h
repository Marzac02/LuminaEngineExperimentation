#pragma once
#include "Assets/Factories/Factory.h"
#include "Memory/RefCounted.h"
#include "Renderer/RHIFwd.h"
#include "Assets/AssetTypes/Textures/Texture.h"
#include "TextureFactory.generated.h"


namespace Lumina
{
    REFLECT()
    class CTextureFactory : public CFactory
    {
        GENERATED_BODY()
    public:

        CObject* CreateNew(const FName& Name, CPackage* Package) override;
        CClass* GetAssetClass() const override { return CTexture::StaticClass(); }
        FString GetAssetName() const override { return "Texture"; }
        FString GetDefaultAssetCreationName(const FString& InPath) override { return "NewTexture"; }

        bool IsExtensionSupported(const FString& Ext) override { return Ext == ".png" || Ext == ".jpg"; }
        bool CanImport() override { return true; }
        
        void TryImport(const FString& RawPath, const FString& DestinationPath) override;

    private:

    };
}
