#include "pch.h"
#include "Package.h"

#include "Assets/AssetRegistry/AssetRegistry.h"
#include "Core/Engine/Engine.h"
#include "Core/Object/Cast.h"
#include "Core/Object/Class.h"
#include "Core/Object/ObjectIterator.h"
#include "Core/Object/Archive/ObjectReferenceReplacerArchive.h"
#include "Core/Profiler/Profile.h"
#include "Paths/Paths.h"
#include "Platform/Filesystem/FileHelper.h"

#include "Core/Serialization/Package/PackageSaver.h"
#include "Core/Serialization/Package/PackageLoader.h"
#include "TaskSystem/TaskSystem.h"
#include "Thumbnail/PackageThumbnail.h"


namespace Lumina
{
    IMPLEMENT_INTRINSIC_CLASS(CPackage, CObject, LUMINA_API)


    FObjectExport::FObjectExport(CObject* InObject)
    {
        ObjectGUID      = InObject->GetGUID();
        ObjectName      = InObject->GetName();
        ClassName       = InObject->GetClass()->GetName();
        Offset          = 0;
        Size            = 0;
        Object          = InObject;
    }

    FObjectImport::FObjectImport(CObject* InObject)
    {
        ObjectGUID      = InObject->GetGUID();
        Object          = InObject;
    }

    
    void CPackage::OnDestroy()
    {
        
    }

    bool CPackage::Rename(const FName& NewName, CPackage* NewPackage)
    {
		FString FileName = Paths::FileName(NewName.ToString(), true);
		FString OldFileName = Paths::FileName(GetName().ToString(), true);
        bool bFileNameDirty = FileName != OldFileName;
     
        if (bFileNameDirty)
        {
            for (FObjectExport& Export : ExportTable)
            {
                if (Export.ObjectName == OldFileName)
                {
                    Export.ObjectName = FileName;
                    if (CObject* Object = Export.Object.Get())
                    {
                        LUM_ASSERT(Object->GetName() == OldFileName)
                            Object->Rename(FileName, nullptr);
                        break;
                    }
                }
            }
        }

		bool bSuccess = CObject::Rename(NewName, NewPackage);
        if (bSuccess && bFileNameDirty)
        {
			FString FilePath = Paths::ResolveVirtualPath(GetName().ToString()) + ".lasset";
            CPackage::SavePackage(this, FilePath);

            for (FObjectExport& Export : ExportTable)
            {
                if (Export.Object.Get())
                {
                    Export.Object.Get()->ConditionalBeginDestroy();
                }
            }
        }


        return bSuccess;
    }

    CPackage* CPackage::CreatePackage(const FString& FileName)
    {
        FString VirtualPath = Paths::ConvertToVirtualPath(FileName.c_str());
        
        CPackage* Package = FindObject<CPackage>(VirtualPath);
        if (Package)
        {
            LOG_WARN("Attempted to create a package that already existed {}", FileName);
            return Package;
        }
        
        Package = NewObject<CPackage>(nullptr, VirtualPath);
        Package->AddToRoot();

        Package->PackageThumbnail = MakeSharedPtr<FPackageThumbnail>();
        
        LOG_INFO("Created Package: \"{}\"", VirtualPath);
        
        Package->MarkDirty();
        
        return Package;
    }
    
    bool CPackage::DestroyPackage(const FString& PackageFilePath)
    {
        FString PackageVirtualPath = Paths::ConvertToVirtualPath(PackageFilePath);

        // If the package is loaded, we need to handle replacing references to its assets.
        if (CPackage* Package = FindObject<CPackage>(PackageVirtualPath))
        {
            return CPackage::DestroyPackage(Package);
        }

        FString PackagePath = PackageFilePath;
        if (!Paths::HasExtension(PackagePath, "lasset"))
        {
            Paths::AddPackageExtension(PackagePath);
        }
        
        TVector<uint8> PackageBlob;
        if (!FileHelper::LoadFileToArray(PackageBlob, PackageFilePath))
        {
            LOG_ERROR("Failed to load package file at path {}", PackagePath);
            return false;
        }

        FName PackageFileName = Paths::FileName(PackagePath, true);


        FPackageHeader Header;
        FMemoryReader Reader(PackageBlob);
        Reader << Header;

        Reader.Seek(Header.ExportTableOffset);
        
        TVector<FObjectExport> Exports;
        Reader << Exports;

        TOptional<FObjectExport> Export;
        for (const FObjectExport& E : Exports)
        {
            if (E.ObjectName == PackageFileName)
            {
                Export = E;
                break;
            }
        }

        if (!Export.has_value())
        {
            LOG_ERROR("No primary asset found in package");
            return false;
        }
        

        FAssetRegistry::Get().AssetDeleted(Export->ObjectGUID);

        try
        {
            std::filesystem::remove(PackagePath.c_str());
        }
        catch (std::filesystem::filesystem_error& Error)
        {
            LOG_ERROR("Failed to delete asset file {0}", Error.what());
            return false;
        }
        
        return true;
    }
    
    bool CPackage::DestroyPackage(CPackage* PackageToDestroy)
    {
        LUM_ASSERT(PackageToDestroy->FullyLoad())

        TVector<CObject*> ExportObjects;
        ExportObjects.reserve(20);
        GetObjectsWithPackage(PackageToDestroy, ExportObjects);

        FGuid AssetGUID;
        for (CObject* ExportObject : ExportObjects)
        {
            if (ExportObject->IsAsset())
            {
                AssetGUID = ExportObject->GetGUID();
                FObjectReferenceReplacerArchive Ar(ExportObject, nullptr);
                for (TObjectIterator<CObject> Itr; Itr; ++Itr)
                {
                    CObject* Object = *Itr;
                    Object->Serialize(Ar);
                }
            }
        
            if (ExportObject != PackageToDestroy)
            {
                ExportObject->ConditionalBeginDestroy();
            }
        }
        
        PackageToDestroy->ExportTable.clear();
        PackageToDestroy->ImportTable.clear();

        FName PackagePath = PackageToDestroy->GetFullPackageFilePath();
        
        PackageToDestroy->RemoveFromRoot();
        PackageToDestroy->ConditionalBeginDestroy();

        FAssetRegistry::Get().AssetDeleted(AssetGUID);

        std::filesystem::remove(PackagePath.c_str());

        return true;
    }

    CPackage* CPackage::FindPackageByPath(const FString& FullPath)
    {
        FString Path = FullPath;
        if (!Paths::HasExtension(Path, "lasset"))
        {
            Paths::AddPackageExtension(Path);
        }

        FString VirtualPath = Paths::ConvertToVirtualPath(Path);
        CPackage* Package = FindObject<CPackage>(VirtualPath);

        return Package;
    }

    void CPackage::RenamePackage(const FString& OldPath, const FString& NewPath)
    {
        if (CPackage* Package = FindObject<CPackage>(OldPath))
        {
            LUM_ASSERT(Package->Rename(NewPath, nullptr))
            return;
        }

		FName NewFileName = Paths::FileName(NewPath, true);
		FName OldFileName = Paths::FileName(OldPath, true);
        bool bFileNameDirty = NewFileName != OldFileName;
        if (!bFileNameDirty)
        {
            return;
		}

		// This is kind of weird but the file has already been moved, so we need to use the new path to load it.
		FString FilePath = Paths::ResolveVirtualPath(NewPath) + ".lasset";
        TVector<uint8> FileBlob;
        if (!FileHelper::LoadFileToArray(FileBlob, FilePath))
        {
			LOG_ERROR("Failed to load package file at path {}", FilePath);
            return;
        }

		FMemoryReader Reader(FileBlob);

        FPackageHeader Header;
        Reader << Header;
        Reader.Seek(Header.ExportTableOffset);
        
        TVector<FObjectExport> Exports;
        Reader << Exports;

        for (FObjectExport& Export : Exports)
        {
            if (Export.ObjectName == OldFileName)
            {
                Export.ObjectName = NewFileName;
                break;
            }
        }

        FMemoryWriter Writer(FileBlob);
        Writer.Seek(Header.ExportTableOffset);
        Writer << Exports;

		FileHelper::SaveArrayToFile(FileBlob, FilePath);
	}

    CPackage* CPackage::LoadPackage(const FName& FileName)
    {
        LUMINA_PROFILE_SCOPE();

        FString FullName = FileName.ToString();
        if (!Paths::HasExtension(FullName, "lasset"))
        {
            Paths::AddPackageExtension(FullName);
        }

        FString VirtualPath = Paths::ConvertToVirtualPath(FullName);
        CPackage* Package = FindObject<CPackage>(VirtualPath.c_str());
        if (Package)
        {
            // Package is already loaded.
            return Package;
        }
        
        TVector<uint8> FileBinary;
        if (!FileHelper::LoadFileToArray(FileBinary, FullName))
        {
            return nullptr;
        }
        
        Package = NewObject<CPackage>(nullptr, VirtualPath.c_str());
        Package->CreateLoader(FileBinary);
        
        FPackageLoader& Reader = *(FPackageLoader*)Package->Loader.get();
        
        FPackageHeader PackageHeader;
        Reader << PackageHeader;

        if (PackageHeader.Tag != PACKAGE_FILE_TAG)
        {
            LOG_ERROR("Failed to load package, invalid data was given");
            Package->Loader.reset();
            return nullptr;
        }

        Reader.Seek(PackageHeader.ImportTableOffset);
        Reader << Package->ImportTable;
        
        Reader.Seek(PackageHeader.ExportTableOffset);
        Reader << Package->ExportTable;
        
        int64 SizeBefore = Reader.Tell();
        Reader.Seek(PackageHeader.ThumbnailDataOffset);
        
        Package->PackageThumbnail = MakeSharedPtr<FPackageThumbnail>();
        Package->PackageThumbnail->Serialize(Reader);

        Reader.Seek(SizeBefore);
        
        LOG_INFO("Loaded Package: \"{}\" - ( [{}] Exports | [{}] Imports | [{}] Bytes)", Package->GetName(), Package->ExportTable.size(), Package->ImportTable.size(), Package->Loader->TotalSize());

        return Package;
    }

    bool CPackage::SavePackage(CPackage* Package, const FName& FileName)
    {
        LUMINA_PROFILE_SCOPE();

        if (Package == nullptr)
        {
            LOG_ERROR("Cannot save a null package! {}", FileName);
            return false;
        }

        Package->FullyLoad();

        FString PathString = FileName.ToString();
        if (!Paths::HasExtension(PathString, "lasset"))
        {
            Paths::AddPackageExtension(PathString);
        }
        
        Package->ExportTable.clear();
        Package->ImportTable.clear();
        
        TVector<uint8> FileBinary;
        FPackageSaver Writer(FileBinary, Package);

        
        FPackageHeader Header;
        Header.Tag = PACKAGE_FILE_TAG;
        Header.Version = 1;

        // Skip the header until we've built the tables.
        Writer.Seek(sizeof(FPackageHeader));
        
        // Build the save context (imports/exports)
        FSaveContext SaveContext(Package);
        Package->BuildSaveContext(SaveContext);

        Package->WriteImports(Writer, Header, SaveContext);
        Package->WriteExports(Writer, Header, SaveContext);
        
        Header.ImportCount = (uint32)Package->ImportTable.size();
        Header.ExportCount = (uint32)Package->ExportTable.size();

        Header.ThumbnailDataOffset = Writer.Tell();
        if (!Package->PackageThumbnail)
        {
            Package->PackageThumbnail = MakeSharedPtr<FPackageThumbnail>();
        }
        
        Package->PackageThumbnail->Serialize(Writer);
        
        Writer.Seek(0);
        Writer << Header;

        // Reload the package loader to match the new file binary.
        Package->CreateLoader(FileBinary);

        if (!FileHelper::SaveArrayToFile(FileBinary, PathString))
        {
            return false;
        }
        
        LOG_INFO("Saved Package: \"{}\" - ( [{}] Exports | [{}] Imports | [{:.2f}] KiB)",
            Package->GetName(),
            Package->ExportTable.size(),
            Package->ImportTable.size(),
            static_cast<double>(FileBinary.size()) / 1024.0);

        Package->ClearDirty();
        
        return true;
    }

    void CPackage::CreateLoader(const TVector<uint8>& FileBinary)
    {
        void* HeapData = Memory::Malloc(FileBinary.size());
        Memory::Memcpy(HeapData, FileBinary.data(), FileBinary.size());
        Loader = MakeUniquePtr<FPackageLoader>(HeapData, FileBinary.size(), this);
    }

    FPackageLoader* CPackage::GetLoader() const
    {
        return (FPackageLoader*)Loader.get();
    }

    void CPackage::BuildSaveContext(FSaveContext& Context)
    {
        TVector<CObject*> ExportObjects;
        ExportObjects.reserve(20);
        GetObjectsWithPackage(this, ExportObjects);

        FSaveReferenceBuilderArchive Builder(&Context);
        for (CObject* Object : ExportObjects)
        {
            Builder << Object;
        }
    }

    void CPackage::CreateExports()
    {
        while (ExportIndex < (int64)ExportTable.size())
        {
            

            ++ExportIndex;
        }
    }

    void CPackage::CreateImports()
    {
        
    }

    void CPackage::WriteImports(FPackageSaver& Ar, FPackageHeader& Header, FSaveContext& SaveContext)
    {
        for (CObject* Import : SaveContext.Imports)
        {
            ImportTable.emplace_back(Import);
        }
        
        Header.ImportTableOffset = Ar.Tell();
        Ar << ImportTable;
        
    }

    void CPackage::WriteExports(FPackageSaver& Ar, FPackageHeader& Header, FSaveContext& SaveContext)
    {
        Header.ObjectDataOffset = Ar.Tell();

        for (CObject* Export : SaveContext.Exports)
        {
            Export->LoaderIndex = FObjectPackageIndex::FromExport((int32)ExportTable.size()).GetRaw();
            ExportTable.emplace_back(Export);
        }

        for (size_t i = 0; i < ExportTable.size(); ++i)
        {
            FObjectExport& Export = ExportTable[i];
            LUM_ASSERT(Export.Object.Get() != nullptr)
            
            Export.Offset = Ar.Tell();
            
            Export.Object.Get()->Serialize(Ar);
            
            Export.Size = Ar.Tell() - Export.Offset;
            
        }
        
        Header.ExportTableOffset = Ar.Tell();
        Ar << ExportTable;
    }

    void CPackage::LoadObject(CObject* Object)
    {
        LUMINA_PROFILE_SCOPE();
        if (!Object || !Object->HasAnyFlag(OF_NeedsLoad))
        {
            return;
        }

        CPackage* ObjectPackage = Object->GetPackage();
        
        // If this object's package comes from somewhere else, load it through there.
        if (ObjectPackage != this)
        {
            ObjectPackage->LoadObject(Object);
            return;
        }

        int32 LoaderIndex = FObjectPackageIndex(Object->LoaderIndex).GetArrayIndex();

        if (LoaderIndex < 0 || LoaderIndex >= (int32)ExportTable.size())
        {
            LOG_ERROR("Invalid loader index {} for object {}", LoaderIndex, Object->GetName());
            return;
        }

        FObjectExport& Export = ExportTable[LoaderIndex];

        if (!Loader)
        {
            LOG_ERROR("No loader set for package {}", GetName().ToString());
            return;
        }

        const int64 SavedPos = Loader->Tell();
        const int64 DataPos = Export.Offset;
        const int64 ExpectedSize = Export.Size;

        if (DataPos < 0 || ExpectedSize <= 0)
        {
            LOG_ERROR("Invalid export data for object {}. Offset: {}, Size: {}", Object->GetName().ToString(), DataPos, ExpectedSize);
            return;
        }
        
        Loader->Seek(DataPos);
        
        Object->PreLoad();
        
        Object->Serialize(*Loader);
        
        const int64 ActualSize = Loader->Tell() - DataPos;
        
        if (ActualSize != ExpectedSize)
        {
            LOG_WARN("Mismatched size when loading object {}: expected {}, got {}", Object->GetName().ToString(), ExpectedSize, ActualSize);
        }
        
        Object->ClearFlags(OF_NeedsLoad);
        Object->SetFlag(OF_WasLoaded);

        Object->PostLoad();

        // Reset the state of the loader to the previous object.
        Loader->Seek(SavedPos);
    }

    CObject* CPackage::LoadObject(const FGuid& GUID)
    {
        for (SIZE_T i = 0; i < ExportTable.size(); ++i)
        {
            FObjectExport& Export = ExportTable[i];

            if (Export.ObjectGUID == GUID)
            {
                CClass* ObjectClass = FindObject<CClass>(Export.ClassName);

                CObject* Object = nullptr;
                Object = FindObjectImpl(Export.ObjectGUID);

                if (Object == nullptr)
                {
                    Object = NewObject(ObjectClass, this, Export.ObjectName, Export.ObjectGUID);
                    Object->SetFlag(OF_NeedsLoad);
                    
                    if (Object->IsAsset())
                    {
                        Object->SetFlag(OF_Public);
                    }
                }
            
                Object->LoaderIndex = FObjectPackageIndex::FromExport(static_cast<int32>(i)).GetRaw();

                Export.Object = Object;

                LoadObject(Object);
                
                return Object;
            }
        }

        return nullptr;
    }

    CObject* CPackage::LoadObjectByName(const FName& Name)
    {
        for (SIZE_T i = 0; i < ExportTable.size(); ++i)
        {
            FObjectExport& Export = ExportTable[i];

            if (Export.ObjectName == Name)
            {
                CClass* ObjectClass = FindObject<CClass>(Export.ClassName);

                CObject* Object = nullptr;
                Object = FindObjectImpl(Export.ObjectGUID);

                if (Object == nullptr)
                {
                    Object = NewObject(ObjectClass, this, NAME_None, Export.ObjectGUID);
                    Object->SetFlag(OF_NeedsLoad);
                    
                    if (Object->IsAsset())
                    {
                        Object->SetFlag(OF_Public);
                    }
                }
            
                Object->LoaderIndex = FObjectPackageIndex::FromExport(static_cast<int32>(i)).GetRaw();

                Export.Object = Object;

                LoadObject(Object);
                
                return Object;
            }
        }

        return nullptr;
    }

    bool CPackage::FullyLoad()
    {
        for (const FObjectExport& Export : ExportTable)
        {
            LoadObject(Export.ObjectGUID);
        }

        return true;
    }

    CObject* CPackage::FindObjectInPackage(const FName& Name)
    {
        for (const FObjectExport& Export : ExportTable)
        {
            if (Export.ObjectName == Name)
            {
                return Export.Object.Get();
            }
        }

        return nullptr;
    }

    CObject* CPackage::IndexToObject(const FObjectPackageIndex& Index)
    {
        if (Index.IsNull())
        {
            return nullptr;
        }
        
        if (Index.IsImport())
        {
            SIZE_T ArrayIndex = Index.GetArrayIndex();
            if (ArrayIndex >= ImportTable.size())
            {
                LOG_WARN("Failed to find an object in the import table {}", ArrayIndex);
                return nullptr;
            }

            FObjectImport& Import = ImportTable[ArrayIndex];
            Import.Object = Lumina::LoadObject<CObject>(Import.ObjectGUID);
            
            return ImportTable[ArrayIndex].Object.Get();
        }

        if (Index.IsExport())
        {
            SIZE_T ArrayIndex = Index.GetArrayIndex();
            if (ArrayIndex >= ExportTable.size())
            {
                LOG_WARN("Failed to find an object in the export table {}", ArrayIndex);
                return nullptr;
            }

            return LoadObject(ExportTable[ArrayIndex].ObjectGUID);
        }
        
        return nullptr;
    }

    FString CPackage::GetPackageFilename() const
    {
        return Paths::FileName(GetName().c_str(), true);
    }

    FString CPackage::GetFullPackageFilePath() const
    {
        FString Path = Paths::ResolveVirtualPath(GetName().ToString());
        Paths::AddPackageExtension(Path);
        return Path;
    }
}
