#pragma once

#include "Core/Object/ObjectMacros.h"
#include "Core/Object/Object.h"
#include "Factory.generated.h"


namespace Lumina
{

    REFLECT()
    class LUMINA_API CFactoryRegistry : public CObject
    {
        GENERATED_BODY()
    public:

        static CFactoryRegistry& Get();
        
        void RegistryFactory(CFactory* Factory);

        const TVector<CFactory*>& GetFactories() const { return Factories; }

        // CDOs are always rooted.
        TVector<CFactory*> Factories;
    };
    
    
    
    REFLECT()
    class LUMINA_API CFactory : public CObject
    {
        GENERATED_BODY()

    public:

        void PostCreateCDO() override;
        
        CObject* TryCreateNew(const FString& Path);
        
        virtual FString GetAssetName() const { return ""; }
        virtual FString GetAssetDescription() const { return ""; }
        virtual CClass* GetAssetClass() const { return nullptr; }
        virtual FString GetDefaultAssetCreationName(const FString& InPath) { return "New_Asset"; }
        
        virtual CObject* CreateNew(const FName& Name, CPackage* Package) { return nullptr; }

        void Import(const FString& ImportFile, const FString& DestinationPath);
        
        virtual bool CanImport() { return false; }
        virtual void TryImport(const FString& ImportFilePath, const FString& DestinationPath) { }
        
        

        virtual bool IsExtensionSupported(const FString& Ext) { return false; }
        
        static bool ShowImportDialogue(CFactory* Factory, const FString& RawPath, const FString& DestinationPath);
        static bool ShowCreationDialogue(CFactory* Factory, const FString& Path);

        virtual bool HasImportDialogue() const { return false; }
        virtual bool HasCreationDialogue() const { return false; }
        
    protected:
        
        virtual bool DrawImportDialogue(const FString& RawPath, const FString& DestinationPath, bool& bShouldClose) { return true; }
        virtual bool DrawCreationDialogue(const FString& Path, bool& bShouldClose) { return true; }
        
    };
}
