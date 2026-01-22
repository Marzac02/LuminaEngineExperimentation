#include "pch.h"
#include "Package.h"

#include <utility>

#include "Assets/AssetRegistry/AssetRegistry.h"
#include "Core/Engine/Engine.h"
#include "Core/Object/Class.h"
#include "Core/Object/ObjectIterator.h"
#include "Core/Object/Archive/ObjectReferenceReplacerArchive.h"
#include "Core/Profiler/Profile.h"
#include "Paths/Paths.h"
#include "Platform/Filesystem/FileHelper.h"

#include "Core/Serialization/Package/PackageSaver.h"
#include "Core/Serialization/Package/PackageLoader.h"
#include "FileSystem/FileSystem.h"
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
		FStringView FileName = FileSystem::FileName(NewName.ToString(), true);
		FStringView OldFileName = FileSystem::FileName(GetName().ToString(), true);
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
                        ASSERT(Object->GetName() == OldFileName);
                        Object->Rename(FileName, nullptr);
                        break;
                    }
                }
            }
        }

		bool bSuccess = CObject::Rename(NewName, NewPackage);
        if (bSuccess && bFileNameDirty)
        {
            SavePackage(this, GetPackagePath());

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

    CPackage* CPackage::CreatePackage(FStringView Path)
    {
        FFixedString ObjectName = SanitizeObjectName(Path);
        ASSERT(FindObject<CPackage>(ObjectName) == nullptr);

        CPackage* Package = NewObject<CPackage>(nullptr, ObjectName);
        Package->AddToRoot();

        Package->PackageThumbnail = MakeShared<FPackageThumbnail>();
        
        LOG_INFO("Created Package: \"{}\"", Path);
        
        Package->MarkDirty();
        
        return Package;
    }
    
    bool CPackage::DestroyPackage(FStringView Path)
    {
        // If the package is loaded, we need to handle replacing references to its assets.
        if (CPackage* Package = FindPackageByPath(Path))
        {
            return DestroyPackage(Package);
        }
        
        TVector<uint8> PackageBlob;
        if (!FileSystem::ReadFile(PackageBlob, Path))
        {
            LOG_ERROR("Failed to load package file at path {}", Path);
            return false;
        }
        
        FPackageHeader Header;
        FMemoryReader Reader(PackageBlob);
        Reader << Header;

        Reader.Seek(Header.ExportTableOffset);
        
        TVector<FObjectExport> Exports;
        Reader << Exports;

        FName PackageFileName = FileSystem::FileName(Path, true);

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
        FileSystem::Remove(Path);

        return true;
    }
    
    bool CPackage::DestroyPackage(CPackage* PackageToDestroy)
    {
        ASSERT(PackageToDestroy->FullyLoad());

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
        
        PackageToDestroy->RemoveFromRoot();
        PackageToDestroy->ConditionalBeginDestroy();

        FAssetRegistry::Get().AssetDeleted(AssetGUID);

        FileSystem::Remove(PackageToDestroy->GetPackagePath());
        return true;
    }

    CPackage* CPackage::FindPackageByPath(FStringView Path)
    {
        FFixedString ObjectName = SanitizeObjectName(Path);
        return FindObject<CPackage>(ObjectName);
    }

    void CPackage::RenamePackage(FStringView OldPath, FStringView NewPath)
    {
        FFixedString OldObjectName = SanitizeObjectName(OldPath);
        
        if (CPackage* Package = FindObject<CPackage>(OldObjectName))
        {
            FFixedString NewObjectName = SanitizeObjectName(NewPath);

            ASSERT(Package->Rename(NewObjectName, nullptr));
            return;
        }

		FName NewFileName = FileSystem::FileName(NewPath, true);
		FName OldFileName = FileSystem::FileName(OldPath, true);
        bool bFileNameDirty = NewFileName != OldFileName;
        if (!bFileNameDirty)
        {
            return;
		}

		// This is kind of weird but the file has already been moved, so we need to use the new path to load it.
        TVector<uint8> FileBlob;
        if (!FileSystem::ReadFile(FileBlob, NewPath))
        {
			LOG_ERROR("Failed to load package file at path {}", NewPath);
            return;
        }

		FMemoryReader Reader(FileBlob);

        FPackageHeader Header;
        Reader << Header;
        Reader.Seek(Header.ExportTableOffset);
        
        TVector<FObjectExport> Exports;
        Reader << Exports;

        FObjectExport* Export = eastl::find_if(Exports.begin(), Exports.end(), [&](const FObjectExport& E)
        {
            return Export->ObjectName == NewFileName;
        });
        
        if (Export)
        {
            Export->ObjectName = NewFileName;

            FMemoryWriter Writer(FileBlob);
            Writer.Seek(Header.ExportTableOffset);
            Writer << Exports;

            FileSystem::WriteFile(NewPath, FileBlob);
        }
	}

    CPackage* CPackage::LoadPackage(FStringView Path)
    {
        LUMINA_PROFILE_SCOPE();

        FFixedString ObjectName = SanitizeObjectName(Path);
        
        CPackage* Package = FindObject<CPackage>(ObjectName);
        if (Package)
        {
            // Package is already loaded.
            return Package;
        }
        
        TVector<uint8> FileBinary;
        if (!FileSystem::ReadFile(FileBinary, Path))
        {
            return nullptr;
        }
        
        Package = NewObject<CPackage>(nullptr, ObjectName);
        Package->CreateLoader(FileBinary);
        
        FPackageLoader& Reader = *static_cast<FPackageLoader*>(Package->Loader.get());
        
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
        
        Package->PackageThumbnail = MakeShared<FPackageThumbnail>();
        Package->PackageThumbnail->Serialize(Reader);

        Reader.Seek(SizeBefore);
        
        LOG_INFO("Loaded Package: \"{}\" - ( [{}] Exports | [{}] Imports | [{}] Bytes)", Package->GetName(), Package->ExportTable.size(), Package->ImportTable.size(), Package->Loader->TotalSize());

        return Package;
    }

    bool CPackage::SavePackage(CPackage* Package, FStringView Path)
    {
        LUMINA_PROFILE_SCOPE();

        ASSERT(Package != nullptr);

        (void)Package->FullyLoad();
        
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
        
        Header.ImportCount = static_cast<int32>(Package->ImportTable.size());
        Header.ExportCount = static_cast<int32>(Package->ExportTable.size());

        Header.ThumbnailDataOffset = Writer.Tell();
        if (!Package->PackageThumbnail)
        {
            Package->PackageThumbnail = MakeShared<FPackageThumbnail>();
        }
        
        Package->PackageThumbnail->Serialize(Writer);
        
        Writer.Seek(0);
        Writer << Header;

        // Reload the package loader to match the new file binary.
        Package->CreateLoader(FileBinary);
        
        if(!FileSystem::WriteFile(Path, FileBinary))
        {
            LOG_ERROR("Failed to save package: {}", Path);
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
        Loader = MakeUnique<FPackageLoader>(HeapData, FileBinary.size(), this);
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
        while (std::cmp_less(ExportIndex, ExportTable.size()))
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

        for (FObjectExport& Export : ExportTable)
        {
            ASSERT(Export.Object.Get() != nullptr);
            
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

        if (LoaderIndex < 0 || std::cmp_greater_equal(LoaderIndex, ExportTable.size()))
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
                CObject* Object = FindObjectImpl(Export.ObjectGUID);

                if (Object == nullptr)
                {
                    CClass* ObjectClass = FindObject<CClass>(Export.ClassName);
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

    FFixedString CPackage::GetPackagePath() const
    {
        FFixedString Path(GetName().c_str(), GetName().Length());
        AddPackageExt(Path);
        
        return Path;
    }
}
