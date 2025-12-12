#include "ContentBrowserEditorTool.h"

#include "EditorToolContext.h"
#include "LuminaEditor.h"
#include "Assets/AssetManager/AssetManager.h"
#include "Assets/AssetRegistry/AssetRegistry.h"
#include "Assets/Factories/Factory.h"
#include "Core/Engine/Engine.h"
#include "Core/Object/ObjectIterator.h"
#include "Core/Object/ObjectRename.h"
#include "Core/Object/Package/Package.h"
#include "Core/Object/Package/Thumbnail/PackageThumbnail.h"
#include "Core/Windows/Window.h"
#include "EASTL/sort.h"
#include "Paths/Paths.h"
#include "Platform/Filesystem/FileHelper.h"
#include "Platform/Process/PlatformProcess.h"
#include "Project/Project.h"
#include "TaskSystem/TaskSystem.h"
#include "Tools/Dialogs/Dialogs.h"
#include "Tools/Import/ImportHelpers.h"
#include "Tools/UI/ImGui/ImGuiFonts.h"
#include "Tools/UI/ImGui/ImGuiX.h"

namespace Lumina
{

    class FRenameModalState
    {
    public:
        
        char Buffer[256] = {};
        bool bInitialized = false;
    
        void Initialize(const FString& CurrentName)
        {
            if (!bInitialized)
            {
                strncpy_s(Buffer, sizeof(Buffer), CurrentName.c_str(), _TRUNCATE);
                bInitialized = true;
            }
        }
    
        void Reset()
        {
            memset(Buffer, 0, sizeof(Buffer));
            bInitialized = false;
        }
    };

    bool FContentBrowserEditorTool::OnEvent(FEvent& Event)
    {
        if (Event.IsA<FFileDropEvent>())
        {
            FFileDropEvent& FileEvent = Event.As<FFileDropEvent>();

            ImVec2 DropCursor = ImVec2(FileEvent.GetMouseX(), FileEvent.GetMouseY());

            for (const FString& Path : FileEvent.GetPaths())
            {
                PendingActions.get<FPendingOSDrop>()
                PendingActions.emplace_back(FPendingOSDrop{ Path, DropCursor });
            }

            return true;
        }

        return false;
    }

    void FContentBrowserEditorTool::RefreshContentBrowser()
    {
        ContentBrowserTileView.MarkTreeDirty();
        OutlinerListView.MarkTreeDirty();
    }

    void FContentBrowserEditorTool::OnInitialize()
    {
        using namespace Import::Textures;

        (void)FAssetRegistry::Get().GetOnAssetRegistryUpdated().AddMember(this, &FContentBrowserEditorTool::RefreshContentBrowser);
        (void)GEditorEngine->GetProject().OnProjectLoaded.AddMember(this, &FContentBrowserEditorTool::OnProjectLoaded);

        if (GEditorEngine->GetProject().HasLoadedProject())
        {
            SelectedPath = GEditorEngine->GetProject().GetProjectContentDirectory();
        }

        const TVector<CFactory*>& Factories = CFactoryRegistry::Get().GetFactories();
        for (CFactory* Factory : Factories)
        {
            if (CClass* AssetClass = Factory->GetAssetClass())
            {
                FilterState.emplace(AssetClass->GetName().c_str(), true);
            }
        }
        
        CreateToolWindow("Content", [this] (const FUpdateContext& Contxt, bool bIsFocused)
        {
            float Left = 200.0f;
            float Right = ImGui::GetContentRegionAvail().x - Left;
            
            DrawDirectoryBrowser(Contxt, bIsFocused, ImVec2(Left, 0));
            
            ImGui::SameLine();

            DrawContentBrowser(Contxt, bIsFocused, ImVec2(Right, 0));
        });
        
        ContentBrowserTileViewContext.DragDropFunction = [this] (FTileViewItem* DropItem)
        {
            auto* TypedDroppedItem = (FContentBrowserTileViewItem*)DropItem;
            if (!TypedDroppedItem->IsDirectory())
            {
                return;
            }
            
            const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(FContentBrowserTileViewItem::DragDropID, ImGuiDragDropFlags_AcceptBeforeDelivery);
            if (Payload && Payload->IsDelivery())
            {
                uintptr_t ValuePtr = *static_cast<uintptr_t*>(Payload->Data);
                auto* SourceItem = reinterpret_cast<FContentBrowserTileViewItem*>(ValuePtr);

                if (SourceItem == TypedDroppedItem)
                {
                    return;
                }

                HandleContentBrowserDragDrop(TypedDroppedItem, SourceItem);
                
            }
        };

        ContentBrowserTileViewContext.DrawItemOverrideFunction = [this] (FTileViewItem* Item) -> bool
        {
            FContentBrowserTileViewItem* ContentItem = static_cast<FContentBrowserTileViewItem*>(Item);
            
            ImVec4 TintColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

            ImTextureRef ImTexture = ImGuiX::ToImTextureRef(Paths::GetEngineResourceDirectory() + "/Textures/File.png");
            
            if (!ContentItem->IsDirectory())
            {
                if (ContentItem->IsAsset())
                {
                    //if (CPackage* Package = ContentItem->GetPackage())
                    //{
                    //    ImTexture = ImGuiX::ToImTextureRef(Paths::GetEngineResourceDirectory() + "/Textures/File.png");
                    //
                    //    //if (Package->GetPackageThumbnail()->LoadedImage)
                    //    //{
                    //    //    ImTexture = ImGuiX::ToImTextureRef(Package->GetPackageThumbnail()->LoadedImage);
                    //    //}
                    //}
                }
            }
            else
            {
                ImTexture = ImGuiX::ToImTextureRef(Paths::GetEngineResourceDirectory() + "/Textures/Folder.png");
                TintColor = ImVec4(1.0f, 0.9f, 0.6f, 1.0f);
            }
            
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.16f, 0.17f, 1.0f)); 
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.22f, 0.24f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.26f, 0.26f, 0.28f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        
            ImDrawList* DrawList = ImGui::GetWindowDrawList();
            ImVec2 Pos = ImGui::GetCursorScreenPos();
            ImVec2 Size = ImVec2(120.0f, 120.0f);
            
            DrawList->AddRectFilled(
                ImVec2(Pos.x + 3, Pos.y + 3),
                ImVec2(Pos.x + Size.x + 11, Pos.y + Size.y + 11),
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.0f, 0.0f, 0.0f, 0.3f)),
                8.0f
            );
            
            bool clicked = ImGui::ImageButton("##", ImTexture, Size, ImVec2(0, 0), ImVec2(1, 1), ImVec4(0, 0, 0, 0), TintColor);
        
            if (ImGui::IsItemHovered())
            {
                DrawList->AddRect(
                    Pos, 
                    ImVec2(Pos.x + Size.x + 8, Pos.y + Size.y + 8), 
                    ImGui::ColorConvertFloat4ToU32(ImVec4(0.4f, 0.6f, 0.9f, 0.7f)), 
                    8.0f, 
                    0, 
                    2.0f
                );
            }
        
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(3);
        
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
            {
                return true;
            }
        
            return clicked;
        };
        
        ContentBrowserTileViewContext.ItemSelectedFunction = [this] (FTileViewItem* Item)
        {
            FContentBrowserTileViewItem* ContentItem = static_cast<FContentBrowserTileViewItem*>(Item);
            if (ContentItem->IsDirectory())
            {
                SelectedPath = ContentItem->GetPath();
                RefreshContentBrowser();
            }
            else if (ContentItem->IsAsset())
            {
                if (const FAssetData* Asset = FAssetQuery().WithPath(ContentItem->GetPath()).ExecuteFirst())
                {
                    ToolContext->OpenAssetEditor(Asset->AssetGUID);
                }
            }
            else if (ContentItem->IsLuaScript())
            {
                ToolContext->OpenScriptEditor(ContentItem->GetPath());
            }
        };
        
        ContentBrowserTileViewContext.DrawItemContextMenuFunction = [this] (const TVector<FTileViewItem*>& Items)
        {
            bool bMultipleItems = Items.size() > 1;
            
            for (FTileViewItem* Item : Items)
            {
                FContentBrowserTileViewItem* ContentItem = static_cast<FContentBrowserTileViewItem*>(Item);

                if (bMultipleItems)
                {
                    continue;
                }

                if (ContentItem->IsAsset())
                {
                    DrawAssetContextMenu(ContentItem);
                }
                else if (ContentItem->IsLuaScript())
                {
                    DrawLuaScriptContextMenu(ContentItem);
                }
                else if (ContentItem->IsDirectory())
                {
                    DrawDirectoryContextMenu(ContentItem);
                }
            }
        };

        ContentBrowserTileViewContext.RebuildTreeFunction = [this] (FTileViewWidget* Tree)
        {
            if (Paths::Exists(SelectedPath))
            {
                TFixedVector<FString, 8> DirectoryPaths;
                TFixedVector<FString, 6> FilePaths;

                for (auto& Directory : std::filesystem::directory_iterator(SelectedPath.c_str()))
                {
                    if (std::filesystem::is_directory(Directory))
                    {
                        FString VirtualPath = Paths::ConvertToVirtualPath(Directory.path().generic_string().c_str());
                        DirectoryPaths.push_back(Directory.path().generic_string().c_str());
                    }
                    else
                    {
                        FilePaths.push_back(Directory.path().generic_string().c_str());
                    }
                }

                eastl::sort(DirectoryPaths.begin(), DirectoryPaths.end());
                eastl::sort(FilePaths.begin(), FilePaths.end());
                
                for (const FString& Directory : DirectoryPaths)
                {
                    ContentBrowserTileView.AddItemToTree<FContentBrowserTileViewItem>(nullptr, Directory);
                }

                for (const FString& FilePath : FilePaths)
                {
                    ContentBrowserTileView.AddItemToTree<FContentBrowserTileViewItem>(nullptr, FilePath);
                }
            }
        };

        OutlinerContext.DrawItemContextMenuFunction = [this](const TVector<FTreeListViewItem*>& Items)
        {
            for (FTreeListViewItem* Item : Items)
            {
                //FContentBrowserListViewItem* ContentItem = static_cast<FContentBrowserListViewItem*>(Item);
                
            }
        };
        
        OutlinerContext.RebuildTreeFunction = [this](FTreeListView* Tree)
        {
            for (const auto& [VirtualPrefix, PhysicalRootStr] : Paths::GetMountedPaths())
            {
                auto* RootItem = OutlinerListView.AddItemToTree<FContentBrowserListViewItem>(nullptr, PhysicalRootStr, VirtualPrefix.ToString());

                TFunction<void(FContentBrowserListViewItem*, const FString&)> AddChildrenRecursive;

                AddChildrenRecursive = [&](FContentBrowserListViewItem* ParentItem, const FString& CurrentPath)
                {
                    std::error_code ec;
                    for (auto& Entry : std::filesystem::directory_iterator(CurrentPath.c_str(), ec))
                    {
                        if (ec || !Entry.is_directory())
                        {
                            continue;
                        }

                        FString Path = Entry.path().generic_string().c_str();
                        FString DisplayName = Entry.path().filename().string().c_str();
                        auto* ChildItem = ParentItem->AddChild<FContentBrowserListViewItem>(ParentItem, Path, DisplayName);

                        if (Entry.path() == SelectedPath.c_str())
                        {
                            OutlinerListView.SetSelection(ChildItem, OutlinerContext);
                        }

                        AddChildrenRecursive(ChildItem, Path);
                    }
                };

                AddChildrenRecursive(RootItem, PhysicalRootStr);
            }
        };



        OutlinerContext.ItemSelectedFunction = [this] (FTreeListViewItem* Item)
        {
            if (Item == nullptr)
            {
                return;
            }
            
            FContentBrowserListViewItem* ContentItem = static_cast<FContentBrowserListViewItem*>(Item);
            
            SelectedPath = ContentItem->GetPath();

            RefreshContentBrowser();
        };

        OutlinerListView.MarkTreeDirty();
        ContentBrowserTileView.MarkTreeDirty();
    }

    void FContentBrowserEditorTool::Update(const FUpdateContext& UpdateContext)
    {
        
    }
    
    void FContentBrowserEditorTool::EndFrame()
    {
        bool bWroteSomething = false;
        

        for (const FPendingDestroy& Destroy : PendingActions.get<FPendingDestroy>())
        {
            try
            {
                if (std::filesystem::is_directory(Destroy.PendingDestroy.c_str()))
                {
                    if (std::filesystem::is_empty(Destroy.PendingDestroy.c_str()))
                    {
                        std::filesystem::remove(Destroy.PendingDestroy.c_str());
                        bWroteSomething = true;
                        
                        ImGuiX::Notifications::NotifySuccess("Deleted Directory {0}", Destroy.PendingDestroy.c_str());
                    }
                }
                else
                {
                    if (const FAssetData* Data = FAssetQuery().WithPath(Destroy.PendingDestroy).ExecuteFirst())
                    {
                        if (CObject* AliveObject = FindObject<CObject>(Data->AssetGUID))
                        {
                            ToolContext->OnDestroyAsset(AliveObject);
                        }
                    }

                    if (CPackage::DestroyPackage(Destroy.PendingDestroy))
                    {
                        FString PackagePathWithExt = Destroy.PendingDestroy + ".lasset";
                        bWroteSomething = true;

                        ImGuiX::Notifications::NotifySuccess("Deleted Asset {0}", Destroy.PendingDestroy.c_str());

                    }
                }
            }
            catch (const std::filesystem::filesystem_error& e)
            {
                LOG_ERROR("Failed to delete file: {0}", e.what());
                ImGuiX::Notifications::NotifyError("Deletion failed: {0}", e.what());
            }
        }

        for (const FPendingRename& Rename : PendingActions.get<FPendingRename>())
        {
            
        }
        
        if (bWroteSomething)
        {
            RefreshContentBrowser();
        }

        PendingDestroy.clear();
        PendingRenames.clear();
    }
    
    void FContentBrowserEditorTool::InitializeDockingLayout(ImGuiID InDockspaceID, const ImVec2& InDockspaceSize) const
    {
        ImGuiID topDockID = 0, bottomLeftDockID = 0, bottomCenterDockID = 0, bottomRightDockID = 0;
        ImGui::DockBuilderSplitNode(InDockspaceID, ImGuiDir_Down, 0.5f, &bottomCenterDockID, &topDockID);
        ImGui::DockBuilderSplitNode(bottomCenterDockID, ImGuiDir_Right, 0.66f, &bottomCenterDockID, &bottomLeftDockID);
        ImGui::DockBuilderSplitNode(bottomCenterDockID, ImGuiDir_Right, 0.5f, &bottomRightDockID, &bottomCenterDockID);

        ImGui::DockBuilderDockWindow(GetToolWindowName("Content").c_str(), bottomCenterDockID);
    }

    void FContentBrowserEditorTool::DrawToolMenu(const FUpdateContext& UpdateContext)
    {
        if (ImGui::BeginMenu(LE_ICON_FILTER " Filter"))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 2));

            for (auto& [Name, State] : FilterState)
            {
                if (ImGui::Checkbox(Name.c_str(), &State))
                {
                    RefreshContentBrowser();
                }
            }

            ImGui::PopStyleVar(2);
            ImGui::EndMenu();
        }
    }

    void FContentBrowserEditorTool::HandleContentBrowserDragDrop(FContentBrowserTileViewItem* Drop, FContentBrowserTileViewItem* Payload)
    {
        /** If the payload is a folder, and is empty, we don't need to do much besides moving the folder */
        if (Payload->IsDirectory() && Paths::Exists(Payload->GetPath()))
        {
            if (std::filesystem::is_empty(Payload->GetPath().c_str()))
            {
                std::filesystem::path SourcePath = Payload->GetPath().c_str();
                FString SourceFileName = SourcePath.filename().generic_string().c_str();
            
                FString DestPath = Drop->GetPath() + "/" + SourceFileName;
                
                try
                {
                    std::filesystem::rename(SourcePath, DestPath.c_str());
                    ImGuiX::Notifications::NotifySuccess("Moved folder {0} \n {1}", SourcePath.string(), DestPath);
                    RefreshContentBrowser();
                }
                catch (const std::filesystem::filesystem_error& e)
                {
                    ImGuiX::Notifications::NotifyError("Failed to move folder: {0}", e.what());
                }
            }
            else
            {
                
                //...TODO Moving all assets in a folder :(
            }
        }


        if (!Payload->IsDirectory())
        {
            const FString& OldPath = Payload->GetPath();
            //FString NewPath = Drop->GetPath() + "/" + Payload->GetAssetData().AssetName.ToString();
            //PendingRenames.emplace_back(FPendingRename{OldPath, NewPath});
        }
    }

    void FContentBrowserEditorTool::OpenDeletionWarningPopup(const FContentBrowserTileViewItem* Item, const TFunction<void(EYesNo)>& Callback)
    {
        if (Dialogs::Confirmation("Confirm Deletion", "Are you sure you want to delete \"{0}\"?\n""\nThis action cannot be undone.", Item->GetFileName()))
        {
            Callback(EYesNo::Yes);
            PendingDestroy.emplace_back(FPendingDestroy{Item->GetPath()});
        }

        Callback(EYesNo::No);
    }

    void FContentBrowserEditorTool::OnProjectLoaded()
    {
        SelectedPath = GEditorEngine->GetProject().GetProjectGameDirectory();
        
        Watcher.Stop();
        Watcher.Watch(GEditorEngine->GetProject().GetProjectScriptsDirectory(), [&](const FFileEvent& Event)
        {
            switch (Event.Action)
            {
            case EFileAction::Added:
                {
                    Scripting::FScriptingContext::Get().OnScriptCreated(Event.Path);
                    RefreshContentBrowser();
                }
                break;
            case EFileAction::Modified:
                {
                    Scripting::FScriptingContext::Get().OnScriptReloaded(Event.Path);
                    RefreshContentBrowser();
                }
                break;
            case EFileAction::Removed:
                {
                    Scripting::FScriptingContext::Get().OnScriptDeleted(Event.Path);
                    RefreshContentBrowser();
                }
                break;
            case EFileAction::Renamed:
                {
                    Scripting::FScriptingContext::Get().OnScriptRenamed(Event.Path, Event.OldPath);
                    RefreshContentBrowser();
                }
                break;
            }
        });
    }
    

    void FContentBrowserEditorTool::TryImport(const FString& Path)
    {
        const TVector<CFactory*>& Factories = CFactoryRegistry::Get().GetFactories();
        for (CFactory* Factory : Factories)
        {
            if (!Factory->CanImport())
            {
                continue;
            }
        
            FString Ext = Paths::GetExtension(Path);
            if (!Factory->IsExtensionSupported(Ext))
            {
                continue;
            }
            
            FString NoExtFileName = Paths::FileName(Path, true);
            FString PathString = Paths::Combine(SelectedPath.c_str(), NoExtFileName.c_str());
        
            Paths::AddPackageExtension(PathString);
            PathString = Paths::MakeUniquePath(PathString);
            PathString = Paths::RemoveExtension(PathString);
        
            if (Factory->HasImportDialogue())
            {
                ToolContext->PushModal("Import", {700, 800}, [this, Factory, Path, PathString](const FUpdateContext&)
                {
                    bool bShouldClose = CFactory::ShowImportDialogue(Factory, Path, PathString);
                    if (bShouldClose)
                    {
                        ImGuiX::Notifications::NotifySuccess("Successfully Imported: \"{0}\"", PathString.c_str());
                    }
            
                    return bShouldClose;
                });
            }
            else
            {
                Task::AsyncTask(1, 1, [this, Factory, Path, PathString] (uint32, uint32, uint32)
                {
                    Factory->Import(Path, PathString);
                    ImGuiX::Notifications::NotifySuccess("Successfully Imported: \"{0}\"", PathString.c_str());
                });
            }
        }
    }
    

    ObjectRename::EObjectRenameResult FContentBrowserEditorTool::HandleRenameEvent(const FString& OldPath, const FString& NewPath)
    {
        return ObjectRename::RenameObject(OldPath, NewPath);
    }

    void FContentBrowserEditorTool::PushRenameModal(FContentBrowserTileViewItem* ContentItem)
    {
        TUniquePtr<FRenameModalState> RenameState = MakeUniquePtr<FRenameModalState>();
        
        ToolContext->PushModal("Rename", ImVec2(480.0f, 340.0f), [this, ContentItem, RenameState = Move(RenameState)](const FUpdateContext&) -> bool
        {
            RenameState->Initialize(ContentItem->GetFileName());
            
            const ImGuiStyle& style = ImGui::GetStyle();
            const float ContentWidth = ImGui::GetContentRegionAvail().x;
            
            ImGuiX::Font::PushFont(ImGuiX::Font::EFont::MediumBold);
            ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.95f, 1.0f), LE_ICON_ARCHIVE_EDIT " Rename %s", ContentItem->IsDirectory() ? "Folder" : "Asset");
            ImGuiX::Font::PopFont();
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Spacing();
            
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.65f, 1.0f));
            ImGui::Text("Current name:");
            ImGui::PopStyleColor();
            
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.85f, 0.85f, 0.9f, 1.0f), "%s", ContentItem->GetFileName().c_str());
            
            ImGui::Spacing();
            ImGui::Spacing();
            
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.65f, 1.0f));
            ImGui::Text("New name:");
            ImGui::PopStyleColor();
            
            ImGui::Spacing();
            
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.2f, 0.2f, 0.25f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.25f, 0.25f, 0.3f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 8.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            
            ImGui::SetNextItemWidth(-1);
            
            bool bSubmitted = ImGui::InputText("##RenameInput", RenameState->Buffer, sizeof(RenameState->Buffer), 
                ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_EnterReturnsTrue);
            
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(3);
            
            ImGui::Spacing();
            ImGui::Spacing();
            
            bool bIsValid = strlen(RenameState->Buffer) > 0;
            bool bNameUnchanged = strcmp(RenameState->Buffer, ContentItem->GetFileName().c_str()) == 0;
            FString ValidationMessage;
            bool bHasError = false;
            
            if (strlen(RenameState->Buffer) > 0)
            {
                if (bNameUnchanged)
                {
                    ValidationMessage = "Name unchanged - please enter a different name";
                    bHasError = true;
                    bIsValid = false;
                }
                else if (!ContentItem->IsDirectory())
                {
                    //FString TestAssetName = ContentItem->GetAssetData().PackageName.ToString() + "." + RenameState->Buffer;
                    //if (CObject* TestObject = FindObject<CObject>(nullptr, TestAssetName))
                    //{
                    //    //ValidationMessage = std::format("Asset already exists: {}", *TestObject->GetQualifiedName()).c_str();
                    //    bHasError = true;
                    //    bIsValid = false;
                    //}
                }
                else if (ContentItem->IsAsset())
                {
                    FString TestPath = ContentItem->GetPath() + "/" + RenameState->Buffer;
                    if (std::filesystem::exists(TestPath.c_str()))
                    {
                        ValidationMessage = std::format("Path already exists: {}", TestPath.c_str()).c_str();
                        bHasError = true;
                        bIsValid = false;
                    }
                }
            }
            
            if (bHasError && !ValidationMessage.empty())
            {
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.3f, 0.1f, 0.1f, 0.3f));
                ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.8f, 0.2f, 0.2f, 0.4f));
                
                ImGui::BeginChild("##ValidationError", ImVec2(-1, 50.0f), true);
                
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                ImGui::Text(LE_ICON_ALERT_OCTAGON);
                ImGui::SameLine();
                ImGui::TextWrapped("%s", ValidationMessage.c_str());
                ImGui::PopStyleColor();
                
                ImGui::EndChild();
                ImGui::PopStyleColor(2);
                ImGui::PopStyleVar(2);
                
                ImGui::Spacing();
            }
            
            if (bSubmitted && bIsValid)
            {
                FString NewPath = Paths::Parent(ContentItem->GetPath()) + "/" + RenameState->Buffer;
                PendingRenames.emplace_back(FPendingRename{ContentItem->GetPath(), NewPath});
                RenameState->Reset();
                return true;
            }
            
            ImGui::Spacing();
            
            float remainingHeight = ImGui::GetContentRegionAvail().y - 40.0f;
            if (remainingHeight > 0)
            {
                ImGui::Dummy(ImVec2(0, remainingHeight));
            }
            
            ImGui::Separator();
            ImGui::Spacing();

            constexpr float ButtonHeight = 32.0f;
            const float ButtonWidth = (ContentWidth - style.ItemSpacing.x) * 0.5f;
            
            if (!bIsValid)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.9f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 1.0f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.45f, 0.85f, 1.0f));
            }
            
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            
            if (ImGui::Button(LE_ICON_CHECK " Rename", ImVec2(ButtonWidth, ButtonHeight)))
            {
                if (bIsValid)
                {
                    FString NewPath = Paths::Parent(ContentItem->GetPath()) + "/" + RenameState->Buffer;
                    PendingRenames.emplace_back(FPendingRename{ContentItem->GetPath(), NewPath});
                    RenameState->Reset();
                    return true;
                }
            }
            
            ImGui::PopStyleVar();
            
            if (!bIsValid)
            {
                ImGui::PopItemFlag();
                ImGui::PopStyleVar();
            }
            else
            {
                ImGui::PopStyleColor(3);
            }
            
            // Cancel button
            ImGui::SameLine();
            
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.22f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.27f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.15f, 0.17f, 1.0f));
            
            if (ImGui::Button(LE_ICON_CANCEL " Cancel", ImVec2(ButtonWidth, ButtonHeight)))
            {
                RenameState->Reset();
                return true;
            }
            
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar();
            
            // ESC to cancel
            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                RenameState->Reset();
                return true;
            }
            
            return false;
        });
    }

    void FContentBrowserEditorTool::DrawDirectoryBrowser(const FUpdateContext& Context, bool bIsFocused, ImVec2 Size)
    {
        ImGui::BeginChild("Directories", Size);

        OutlinerListView.Draw(OutlinerContext);
        
        ImGui::EndChild();
    }

    void FContentBrowserEditorTool::DrawContentBrowser(const FUpdateContext& Context, bool bIsFocused, ImVec2 Size)
    {
        constexpr float Padding = 10.0f;

        ImVec2 AdjustedSize = ImVec2(Size.x - 2 * Padding, Size.y - 2 * Padding);

        ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(Padding, Padding));

        ImGui::BeginChild("Content", AdjustedSize, true, ImGuiWindowFlags_None);
        
        if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        {
            ImGui::OpenPopup("ContentContextMenu");
        }

        ImGui::SetNextWindowSizeConstraints(ImVec2(200.0f, 100.0f), ImVec2(0.0f, 0.0f));
        
        if (ImGui::BeginPopup("ContentContextMenu"))
        {
            const char* FolderIcon = LE_ICON_FOLDER;
            FString MenuItemName = FString(FolderIcon) + " " + "New Folder";
            if (ImGui::MenuItem(MenuItemName.c_str()))
            {
                FString PathString = FString(SelectedPath + "/NewFolder");
                PathString = Paths::MakeUniquePath(PathString);
                std::filesystem::create_directory(PathString.c_str());
                RefreshContentBrowser();
            }

            if (Paths::IsUnderDirectory(GEditorEngine->GetProject().GetProjectScriptsDirectory(), SelectedPath))
            {
                DrawScriptsDirectoryContextMenu();
            }
            else
            {
                DrawContentDirectoryContextMenu();
            }
            
            ImGui::EndPopup();
        }
        

        ImGui::BeginHorizontal("Breadcrumbs");

        auto gameDirPos = SelectedPath.find("Game");
        if (gameDirPos != std::string::npos)
        {
            FString BasePathStr = SelectedPath.substr(0, gameDirPos);
            std::filesystem::path BasePath(BasePathStr.c_str());
            std::filesystem::path RelativePath = std::filesystem::path(SelectedPath.c_str()).lexically_relative(BasePath);
    
            std::filesystem::path BuildingPath = BasePath;
    
            for (auto it = RelativePath.begin(); it != RelativePath.end(); ++it)
            {
                BuildingPath /= *it;
        
                ImGui::PushID((int)std::distance(RelativePath.begin(), it));
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 0));
            
                    if (ImGui::Button(it->string().c_str()))
                    {
                        SelectedPath = BuildingPath.generic_string().c_str();
                        ContentBrowserTileView.MarkTreeDirty();
                    }
            
                    ImGui::PopStyleVar(2);
                }
                ImGui::PopID();
        
                if (std::next(it) != RelativePath.end())
                {
                    ImGui::TextUnformatted(LE_ICON_ARROW_RIGHT);
                }
            }
        }

        ImGui::EndHorizontal();

        ImGuiTextFilter SearchFilter;
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
        SearchFilter.Draw("##Search", 450.0f);
        ImGui::PopStyleVar();

        ImGui::Separator();
        
        ContentBrowserTileView.Draw(ContentBrowserTileViewContext);

        ImVec2 ChildMin = ImGui::GetWindowPos();
        ImVec2 ChildMax = ImVec2(ChildMin.x + ImGui::GetWindowWidth(), ChildMin.y + ImGui::GetWindowHeight());
        
        ImRect Rect(ChildMin, ChildMax);

        for (FPendingOSDrop& Drop : PendingActions.get<FPendingOSDrop>())
        {
            if (Rect.Contains(Drop.MousePos))
            {
                TryImport(Drop.Path);
            }
        }

        PendingDrops.clear();
        
        ImGui::EndChild();
    
    }

    void FContentBrowserEditorTool::DrawDirectoryContextMenu(FContentBrowserTileViewItem* ContentItem)
    {
        ImGui::Separator();
        
        if (ImGui::MenuItem(LE_ICON_ARCHIVE_EDIT " Rename", "F2"))
        {
            PushRenameModal(ContentItem);
        }
        
        if (ImGui::MenuItem(LE_ICON_FOLDER " Show in Explorer", nullptr, false))
        {
            FString ParentPath = ContentItem->GetPath();
            ParentPath = Paths::Parent(ParentPath);
            Platform::LaunchURL(StringUtils::ToWideString(ParentPath).c_str());
        }

        if (ImGui::MenuItem(LE_ICON_CONTENT_COPY " Copy Path", nullptr, false))
        {
            ImGui::SetClipboardText(ContentItem->GetPath().c_str());
            ImGuiX::Notifications::NotifyInfo("Path copied to clipboard");
        }

        ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255));
        bool bDeleteClicked = ImGui::MenuItem(LE_ICON_ALERT_OCTAGON " Delete", "Del");
        ImGui::PopStyleColor();

        if (bDeleteClicked)
        {
            OpenDeletionWarningPopup(ContentItem);
        }
    }

    void FContentBrowserEditorTool::DrawLuaScriptContextMenu(FContentBrowserTileViewItem* ContentItem)
    {
        ImGui::Separator();

        if (ImGui::MenuItem(LE_ICON_ARCHIVE_EDIT " Rename", "F2"))
        {
            PushRenameModal(ContentItem);
        }
        
        if (ImGui::MenuItem(LE_ICON_FOLDER " Show in Explorer", nullptr, false))
        {
            FString ParentPath = ContentItem->GetPath();
            ParentPath = Paths::Parent(ParentPath);
            Platform::LaunchURL(StringUtils::ToWideString(ParentPath).c_str());
        }
        
        if (ImGui::MenuItem(LE_ICON_CONTENT_COPY " Copy Path", nullptr, false))
        {
            ImGui::SetClipboardText(ContentItem->GetPath().c_str());
            ImGuiX::Notifications::NotifyInfo("Path copied to clipboard");
        }
        
        ImGui::Separator();

        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255));
        bool bDeleteClicked = ImGui::MenuItem(LE_ICON_ALERT_OCTAGON " Delete", "Del");
        ImGui::PopStyleColor();

        if (bDeleteClicked)
        {
            OpenDeletionWarningPopup(ContentItem);
        }
    }

    void FContentBrowserEditorTool::DrawAssetContextMenu(FContentBrowserTileViewItem* ContentItem)
    {
        if (ImGui::MenuItem(LE_ICON_ARCHIVE_EDIT " Rename", "F2"))
        {
            PushRenameModal(ContentItem);
        }
                    
        ImGui::Separator();
        
        if (ImGui::MenuItem(LE_ICON_FOLDER " Show in Explorer", nullptr, false))
        {
            FString PackagePath = ContentItem->GetPath();
            PackagePath = Paths::Parent(PackagePath);
            Platform::LaunchURL(StringUtils::ToWideString(PackagePath).c_str());
        }
        
        if (ImGui::MenuItem(LE_ICON_CONTENT_COPY " Copy Path", nullptr, false))
        {
            ImGui::SetClipboardText(ContentItem->GetPath().c_str());
            ImGuiX::Notifications::NotifyInfo("Path copied to clipboard");
        }
        
        ImGui::Separator();
        
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255));
        bool bDeleteClicked = ImGui::MenuItem(LE_ICON_ALERT_OCTAGON " Delete", "Del");
        ImGui::PopStyleColor();
        
        if (bDeleteClicked)
        {
            OpenDeletionWarningPopup(ContentItem);
        }

        if (ContentItem->IsAsset())
        {
            if (ImGui::MenuItem(LE_ICON_FOLDER_OPEN " Open", "Double-Click"))
            {
                if (const FAssetData* Data = FAssetQuery().WithPath(ContentItem->GetPath()).ExecuteFirst())
                {
                    ToolContext->OpenAssetEditor(Data->AssetGUID);
                }
            }
            ImGui::Separator();
        }
    }


    void FContentBrowserEditorTool::DrawScriptsDirectoryContextMenu()
    {
        if (ImGui::MenuItem(LE_ICON_OPEN_IN_NEW " New Script"))
        {
            FString NewScriptPath = SelectedPath + "/" + "NewScript.lua";
            NewScriptPath = Paths::MakeUniquePath(NewScriptPath);
            
            FileHelper::CreateNewFile(NewScriptPath);

            Platform::LaunchURL(StringUtils::ToWideString(NewScriptPath.data()).c_str());

        }
    }

    void FContentBrowserEditorTool::DrawContentDirectoryContextMenu()
    {
        const char* ImportIcon = LE_ICON_FILE_IMPORT;
        FString MenuItemName = FString(ImportIcon) + " " + "Import Asset";
        if (ImGui::MenuItem(MenuItemName.c_str()))
        {
            FString SelectedFile;
            const char* Filter = "Supported Assets (*.png;*.jpg;*.fbx;*.gltf;*.glb)\0*.png;*.jpg;*.fbx;*.gltf;*.glb\0All Files (*.*)\0*.*\0";
            if (Platform::OpenFileDialogue(SelectedFile, "Import Asset", Filter))
            {
                TryImport(SelectedFile);
            }
        }

        const char* FileIcon = LE_ICON_PLUS;
        const char* File = "New Asset";
        
        ImGui::Separator();
        
        FString FileName = FString(FileIcon) + " " + File;
        
        if (ImGui::BeginMenu(FileName.c_str()))
        {
            const TVector<CFactory*>& Factories = CFactoryRegistry::Get().GetFactories();
            for (CFactory* Factory : Factories)
            {
                if (Factory->CanImport() || Factory->GetAssetClass() == nullptr)
                {
                    continue;
                }
                
                FString DisplayName = Factory->GetAssetName();
                if (ImGui::MenuItem(DisplayName.c_str()))
                {
                    FString PathString = Paths::Combine(SelectedPath.c_str(), Factory->GetDefaultAssetCreationName(PathString).c_str());
                    Paths::AddPackageExtension(PathString);
                    PathString = Paths::MakeUniquePath(PathString);
                    PathString = Paths::RemoveExtension(PathString);
                    
                    if (Factory->HasCreationDialogue())
                    {
                        ToolContext->PushModal("Create New", {500, 500}, [this, Factory, PathString](const FUpdateContext& DrawContext)
                        {
                            bool bShouldClose = CFactory::ShowCreationDialogue(Factory, PathString);
                            if (bShouldClose)
                            {
                                ImGuiX::Notifications::NotifySuccess("Successfully Created: \"{0}\"", PathString.c_str());
                            }
                    
                            return bShouldClose;
                        });
                    }
                    else
                    {
                        if (CObject* Object = Factory->TryCreateNew(PathString))
                        {
                            CPackage* Package = CPackage::FindPackageByPath(PathString);
                            CPackage::SavePackage(Package, PathString);
                            FAssetRegistry::Get().AssetCreated(Object);

                            ImGuiX::Notifications::NotifySuccess("Successfully Created: \"{0}\"", PathString.c_str());
                        }
                        else
                        {
                            ImGuiX::Notifications::NotifyError("Failed to create new: \"{0}\"", PathString.c_str());
        
                        }
                    }
                }
            }
            
            ImGui::EndMenu();
        }
    }
}
