#include "pch.h"
#include "Factory.h"

#include "Assets/AssetRegistry/AssetRegistry.h"
#include "Core/Engine/Engine.h"
#include "Core/Object/Package/Package.h"
#include "Paths/Paths.h"
#include "TaskSystem/TaskSystem.h"

namespace Lumina
{

    static CFactoryRegistry* FactoryRegistry = nullptr;
    
    CFactoryRegistry& CFactoryRegistry::Get()
    {
        static std::once_flag Flag;
        std::call_once(Flag, []()
        {
            FactoryRegistry = NewObject<CFactoryRegistry>();
            FactoryRegistry->AddToRoot();
        });

        return *FactoryRegistry;
    }

    void CFactoryRegistry::RegistryFactory(CFactory* Factory)
    {
        Factories.push_back(Factory);
    }

    void CFactory::PostCreateCDO()
    {
        CFactoryRegistry::Get().RegistryFactory(this);
    }
    
    CObject* CFactory::TryCreateNew(const FString& Path)
    {
        CPackage* Package = CPackage::CreatePackage(Path);
        FString FileName = Paths::FileName(Path, true);

        CObject* New = CreateNew(FileName.c_str(), Package);
        Package->ExportTable.emplace_back(New);
        
        New->SetFlag(OF_Public);

        return New;
    }
    
    void CFactory::Import(const FString& ImportFile, const FString& DestinationPath, const eastl::any& ImportSettings)
    {
        TryImport(ImportFile, DestinationPath, ImportSettings);
    }

    bool CFactory::ShowImportDialogue(CFactory* Factory, const FString& RawPath, const FString& DestinationPath)
    {
        bool bShouldClose = false;
        bool bShouldReimport = true;
        
        eastl::any ImportSettings;
        if (Factory->DrawImportDialogue(RawPath, DestinationPath, ImportSettings, bShouldClose, bShouldReimport))
        {
            Task::AsyncTask(1, 1, [Factory, RawPath, DestinationPath, ImportSettings = Move(ImportSettings)](uint32, uint32, uint32)
            {
                Factory->Import(RawPath, DestinationPath, ImportSettings);
            });
            
            return true;
        }

        return bShouldClose;
    }

    bool CFactory::ShowCreationDialogue(CFactory* Factory, const FString& Path)
    {
        bool bShouldClose = false;
        if (Factory->DrawCreationDialogue(Path, bShouldClose))
        {
            Task::AsyncTask(1, 1, [Factory, Path = Move(Path)](uint32, uint32, uint32)
            {
                Factory->TryCreateNew(Path);
                CPackage* Package = CPackage::FindPackageByPath(Path);
                CPackage::SavePackage(Package, Path);
            });
            
            return true;
        }

        return bShouldClose;
    }
}
