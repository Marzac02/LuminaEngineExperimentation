#include "ContentBrowserEditorTool.h"

#include "EditorToolContext.h"
#include "LuminaEditor.h"
#include "Assets/AssetManager/AssetManager.h"
#include "Assets/AssetRegistry/AssetRegistry.h"
#include "Assets/Factories/Factory.h"
#include "Core/Engine/Engine.h"
#include "Core/Object/ObjectIterator.h"
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

    template<size_t BufferSize = 42>
    class FRenameModalState
    {
    public:
        
        void Initialize(const FString& CurrentName)
        {
            Buffer.assign(CurrentName.begin(), CurrentName.end());
        }

        
        NODISCARD constexpr size_t Capacity() const { return BufferSize; }
        FORCEINLINE NODISCARD char* CStr() { return Buffer.data(); }
        FORCEINLINE NODISCARD bool IsValid() const { return !Buffer.empty(); }
        
    private:
        
        TFixedString<BufferSize> Buffer;
    };

    bool FContentBrowserEditorTool::OnEvent(FEvent& Event)
    {
        if (Event.IsA<FFileDropEvent>())
        {
            FFileDropEvent& FileEvent = Event.As<FFileDropEvent>();

            ImVec2 DropCursor = ImVec2(FileEvent.GetMouseX(), FileEvent.GetMouseY());

            for (const FString& Path : FileEvent.GetPaths())
            {
                ActionRegistry.EnqueueAction<FPendingOSDrop>(FPendingOSDrop{ Path, DropCursor });
            }

            return true;
        }

        return false;
    }

    void FContentBrowserEditorTool::RefreshContentBrowser()
    {
        ContentBrowserTileView.MarkTreeDirty();
        DirectoryListView.MarkTreeDirty();
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
        
        CreateToolWindow("Content", [&] (bool bIsFocused)
        {
            float Left = 200.0f;
            float Right = ImGui::GetContentRegionAvail().x - Left;
            
            DrawDirectoryBrowser(bIsFocused, ImVec2(Left, 0));
            
            ImGui::SameLine();

            DrawContentBrowser(bIsFocused, ImVec2(Right, 0));
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

                HandleContentBrowserDragDrop(TypedDroppedItem->GetPath(), SourceItem->GetPath());
            }
        };

        ContentBrowserTileViewContext.DrawItemOverrideFunction = [this] (FTileViewItem* Item) -> bool
        {
            FContentBrowserTileViewItem* ContentItem = static_cast<FContentBrowserTileViewItem*>(Item);
            
            ImVec4 TintColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

            ImTextureRef ImTexture = ImGuiX::ToImTextureRef(Paths::GetEngineResourceDirectory() + "/Textures/File.png");
            
            if (ContentItem->IsDirectory())
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

                for (const std::filesystem::directory_entry& Directory : std::filesystem::directory_iterator(SelectedPath.c_str()))
                {
                    if (std::filesystem::is_directory(Directory))
                    {
                        FString VirtualPath = Paths::ConvertToVirtualPath(Directory.path().generic_string().c_str());
                        DirectoryPaths.push_back(Directory.path().generic_string().c_str());
                    }
					else if (Directory.path().extension() == ".lasset" || Directory.path().extension() == ".lua")
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

        ContentBrowserTileViewContext.KeyPressedFunction = [this] (FTileViewItem& Item, ImGuiKey Key) -> bool
        {
            if (Key == ImGuiKey_F2)
            {
                FContentBrowserTileViewItem* ContentItem = static_cast<FContentBrowserTileViewItem*>(&Item);
                PushRenameModal(ContentItem);
                return true;
            }

            if (Key == ImGuiKey_Delete)
            {
                FContentBrowserTileViewItem* ContentItem = static_cast<FContentBrowserTileViewItem*>(&Item);
                OpenDeletionWarningPopup(ContentItem);
                return true;
            }

            return false;
        };
        
        DirectoryContext.ItemContextMenuFunction = [this](FTreeListView& Tree, entt::entity Item)
        {
            
        };

        DirectoryContext.DragDropFunction = [this](FTreeListView& Tree, entt::entity Item)
        {
            FContentBrowserListViewItemData& Data = Tree.Get<FContentBrowserListViewItemData>(Item);
            const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(FContentBrowserTileViewItem::DragDropID, ImGuiDragDropFlags_AcceptBeforeDelivery);
            if (Payload && Payload->IsDelivery())
            {
                uintptr_t ValuePtr = *static_cast<uintptr_t*>(Payload->Data);
                auto* SourceItem = reinterpret_cast<FContentBrowserTileViewItem*>(ValuePtr);
                
                HandleContentBrowserDragDrop(Data.Path, SourceItem->GetPath());
            }
        };
        
        DirectoryContext.RebuildTreeFunction = [this](FTreeListView& Tree)
        {
            for (const auto& [VirtualPrefix, PhysicalRootStr] : Paths::GetMountedPaths())
            {
                TFunction<void(entt::entity, const FFixedString&)> AddChildrenRecursive;

                AddChildrenRecursive = [&](entt::entity ParentItem, const FFixedString& CurrentPath)
                {
                    std::error_code ec;
                    for (auto& Entry : std::filesystem::directory_iterator(CurrentPath.c_str(), ec))
                    {
                        if (ec || !Entry.is_directory())
                        {
                            continue;
                        }

                        FFixedString Path = Entry.path().generic_string().c_str();
                        FFixedString FileName = Entry.path().filename().string().c_str();
                        
                        FFixedString DisplayName;
                        DisplayName.append(LE_ICON_FOLDER).append(" ").append(FileName);

                        entt::entity ItemEntity = Tree.CreateNode(ParentItem, DisplayName, Hash::GetHash64(Path));
                        Tree.EmplaceUserData<FContentBrowserListViewItemData>(ItemEntity).Path = Path;

                        if (Entry.path() == SelectedPath.c_str())
                        {
                            FTreeNodeState& State = Tree.Get<FTreeNodeState>(ItemEntity);
                            State.bSelected = true;
                        }

                        AddChildrenRecursive(ItemEntity, Path);
                    }
                };

                FFixedString Name;
                Name.append(LE_ICON_FOLDER).append(" ").append(VirtualPrefix.c_str());
                entt::entity RootItem = Tree.CreateNode(entt::null, Name);
                Tree.EmplaceUserData<FContentBrowserListViewItemData>(RootItem).Path = PhysicalRootStr.c_str();

                AddChildrenRecursive(RootItem, PhysicalRootStr.c_str());
            }
        };

        DirectoryContext.ItemSelectedFunction = [this] (FTreeListView& Tree, entt::entity Item)
        {
            if (Item == entt::null)
            {
                return;
            }
            
            FContentBrowserListViewItemData& Data = Tree.Get<FContentBrowserListViewItemData>(Item);
            
            SelectedPath = Data.Path;

            RefreshContentBrowser();
        };

        DirectoryContext.KeyPressedFunction = [this] (FTreeListView& Tree, entt::entity Item, ImGuiKey Key) -> bool
        {
            return false;
        };
        
        DirectoryListView.MarkTreeDirty();
        ContentBrowserTileView.MarkTreeDirty();
    }

    void FContentBrowserEditorTool::Update(const FUpdateContext& UpdateContext)
    {
        
    }
    
    void FContentBrowserEditorTool::EndFrame()
    {
        bool bWroteSomething = false;
        
        ActionRegistry.ProcessAllOf<FPendingDestroy>([&] (const FPendingDestroy& Destroy)
        {
            try
            {
                if (std::filesystem::is_directory(Destroy.PendingDestroy.c_str()))
                {
                    std::filesystem::remove_all(Destroy.PendingDestroy.c_str());
                    ImGuiX::Notifications::NotifySuccess("Deleted Directory {0}", Destroy.PendingDestroy.c_str());
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
                        ImGuiX::Notifications::NotifySuccess("Deleted Asset {0}", Destroy.PendingDestroy.c_str());

                    }
                }

                bWroteSomething = true;
            }
            catch (const std::filesystem::filesystem_error& e)
            {
                LOG_ERROR("Failed to delete file: {0}", e.what());
                ImGuiX::Notifications::NotifyError("Deletion failed: {0}", e.what());
            }
		});
        
        ActionRegistry.ProcessAllOf<FPendingRename>([&](FPendingRename& Rename)
        {
            try
            {
                std::filesystem::rename(Rename.OldName.c_str(), Rename.NewName.c_str());

                FString Extension = Paths::GetExtension(Rename.OldName);

                // Lua will update itself via the directory watcher.

                if (Extension == ".lasset")
                {
                    // Rename the underlying asset's package.
                    FString OldObjectName = Paths::ConvertToVirtualPath(Rename.OldName);
                    FString NewObjectName = Paths::ConvertToVirtualPath(Rename.NewName);

                    CPackage::RenamePackage(OldObjectName, NewObjectName);
                    FAssetRegistry::Get().AssetRenamed(Rename.OldName, Rename.NewName);
                }
                else if (Extension.empty())
                {
                    for (const std::filesystem::directory_entry& Directory : std::filesystem::recursive_directory_iterator(Rename.NewName.c_str()))
                    {
                        if (Directory.is_directory())
                        {
                            continue;
                        }


                        if (Directory.path().extension() == ".lasset")
                        {
                            FString CurrentPath = Directory.path().string().c_str();

                            FString CurrentVirtualPath = Paths::ConvertToVirtualPath(CurrentPath);

                            FString OldDirVirtual = Paths::ConvertToVirtualPath(Rename.OldName);
                            FString NewDirVirtual = Paths::ConvertToVirtualPath(Rename.NewName);

                            FString OldObjectName = CurrentVirtualPath;
                            if (CurrentVirtualPath.find(NewDirVirtual) == 0)
                            {
                                OldObjectName = OldDirVirtual + CurrentVirtualPath.substr(NewDirVirtual.length());
                            }

                            CPackage::RenamePackage(OldObjectName, CurrentVirtualPath);

                            FString OldObjectFilePath = Paths::ResolveVirtualPath(OldObjectName) += ".lasset";
                            Paths::NormalizePath(CurrentPath);
                            FAssetRegistry::Get().AssetRenamed(OldObjectFilePath, CurrentPath);
                        }
                    }
                }

                ImGuiX::Notifications::NotifySuccess("Rename Success");
                bWroteSomething = true;

            }
            catch (std::filesystem::filesystem_error& Error)
            {
                LOG_ERROR("Failed to process rename: {0}", Error.what());
            }
        });


        if (bWroteSomething)
        {
            RefreshContentBrowser();
        }
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

    void FContentBrowserEditorTool::HandleContentBrowserDragDrop(FStringView DropPath, FStringView PayloadPath)
    {
        size_t Pos = PayloadPath.find_last_of('/');
        FString DirName = (Pos != FString::npos) ? FString(PayloadPath.substr(Pos + 1)) : FString(PayloadPath);
        FString DestPath = FString(DropPath) + "/" + DirName;

        ActionRegistry.EnqueueAction<FPendingRename>(FPendingRename{ FString(PayloadPath), DestPath });
    }

    void FContentBrowserEditorTool::OpenDeletionWarningPopup(const FContentBrowserTileViewItem* Item, const TFunction<void(EYesNo)>& Callback)
    {
        if (Dialogs::Confirmation("Confirm Deletion", "Are you sure you want to delete \"{0}\"?\n""\nThis action cannot be undone.", Item->GetFileName()))
        {
            Callback(EYesNo::Yes);
            ActionRegistry.EnqueueAction<FPendingDestroy>(FPendingDestroy{ Item->GetPath() });
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

    void FContentBrowserEditorTool::PushRenameModal(FContentBrowserTileViewItem* ContentItem)
    {
        ToolContext->PushModal("Rename", ImVec2(480.0f, 320.0f), [this, ContentItem, RenameState = MakeUniquePtr<FRenameModalState<>>()](const FUpdateContext&) -> bool
        {
            RenameState->Initialize(ContentItem->GetFileName());
            
            const ImGuiStyle& style = ImGui::GetStyle();
            const float ContentWidth = ImGui::GetContentRegionAvail().x;
            
            ImGuiX::Font::PushFont(ImGuiX::Font::EFont::MediumBold);
            ImGuiX::TextColored(ImVec4(0.9f, 0.9f, 0.95f, 1.0f), LE_ICON_ARCHIVE_EDIT " Rename {0}", ContentItem->IsDirectory() ? "Folder" : "Asset");
            ImGuiX::Font::PopFont();
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Spacing();
            
            ImGuiX::TextColored(ImVec4(0.6f, 0.6f, 0.65f, 1.0f), "Current name:");
            
            ImGui::SameLine();
            
            ImGuiX::TextColored(ImVec4(0.85f, 0.85f, 0.9f, 1.0f), "{0}", ContentItem->GetFileName().c_str());
            
            ImGui::Spacing();
            ImGui::Spacing();
            
            ImGuiX::TextColoredUnformatted(ImVec4(0.6f, 0.6f, 0.65f, 1.0f), "New name:");
            
            ImGui::Spacing();
            
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.18f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.2f, 0.2f, 0.25f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.25f, 0.25f, 0.3f, 1.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 8.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            
            ImGui::SetNextItemWidth(-1);
            
            bool bSubmitted = ImGui::InputText("##RenameInput", RenameState->CStr(), RenameState->Capacity(), ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_EnterReturnsTrue);
            
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(3);
            
            ImGui::Spacing();
            ImGui::Spacing();
            
            bool bIsValid = RenameState->IsValid();
            bool bNameUnchanged = strcmp(RenameState->CStr(), ContentItem->GetFileName().c_str()) == 0;
            FString ValidationMessage;
            bool bHasError = false;
            
            if (RenameState->IsValid())
            {
                if (bNameUnchanged)
                {
                    ValidationMessage = "Name unchanged - please enter a different name";
                    bHasError = true;
                    bIsValid = false;
                }
                else
                {
                    FString Extension = ContentItem->GetExtension();
                    FString PathNoExt = Paths::RemoveExtension(ContentItem->GetPath());
                    FString TestPath = PathNoExt + "/" + RenameState->CStr() + Extension;

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
                
                ImGui::BeginChild("##ValidationError", ImVec2(-1, 45.0f), true);
                
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
                FString Extension = ContentItem->GetExtension();
                FString ParentPath = Paths::Parent(ContentItem->GetPath());
                FString NewPath = ParentPath + "/" + RenameState->CStr() + Extension;
                ActionRegistry.EnqueueAction<FPendingRename>(FPendingRename{ ContentItem->GetPath(), NewPath });
                return true;
            }
            
            ImGui::Spacing();
            
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
                    FString Extension = ContentItem->GetExtension();
                    FString ParentPath = Paths::Parent(ContentItem->GetPath());
                    FString NewPath = ParentPath + "/" + RenameState->CStr() + Extension;
                    ActionRegistry.EnqueueAction<FPendingRename>(FPendingRename{ ContentItem->GetPath(), NewPath });
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
            
            ImGui::SameLine();
            
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.22f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.27f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.15f, 0.17f, 1.0f));
            
            if (ImGui::Button(LE_ICON_CANCEL " Cancel", ImVec2(ButtonWidth, ButtonHeight)))
            {
                return true;
            }
            
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar();
            
            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                return true;
            }
            
            return false;
        });
    }

    void FContentBrowserEditorTool::DrawDirectoryBrowser(bool bIsFocused, ImVec2 Size)
    {
        ImGui::BeginChild("Directories", Size);

        DirectoryListView.Draw(DirectoryContext);
        
        ImGui::EndChild();
    }

    void FContentBrowserEditorTool::DrawContentBrowser(bool bIsFocused, ImVec2 Size)
    {
        constexpr float Padding = 10.0f;

        ImVec2 AdjustedSize = ImVec2(Size.x - 2 * Padding, 0.0f);

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

        ImGui::Separator();
        
        ContentBrowserTileView.Draw(ContentBrowserTileViewContext);

        ImVec2 ChildMin = ImGui::GetWindowPos();
        ImVec2 ChildMax = ImVec2(ChildMin.x + ImGui::GetWindowWidth(), ChildMin.y + ImGui::GetWindowHeight());
        
        ImRect Rect(ChildMin, ChildMax);

        ActionRegistry.ProcessAllOf<FPendingOSDrop>([&](const FPendingOSDrop& Drop)
        {
            if (Rect.Contains(Drop.MousePos))
            {
                TryImport(Drop.Path);
            }
		});
        
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
            const char* Filter = "Supported Assets (*.png;*.jpg;*.fbx;*.gltf;*.glb;*.obj)\0*.png;*.jpg;*.fbx;*.gltf;*.glb;*.obj\0All Files (*.*)\0*.*\0";
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
