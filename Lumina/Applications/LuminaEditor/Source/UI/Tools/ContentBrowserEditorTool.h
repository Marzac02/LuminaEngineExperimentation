#pragma once
#include "EditorTool.h"
#include "Assets/AssetRegistry/AssetData.h"
#include "Core/LuminaCommonTypes.h"
#include "Core/Object/ObjectRename.h"
#include "Core/Object/Package/Package.h"
#include "FileSystem/FileSystem.h"
#include "Paths/Paths.h"
#include "Platform/Filesystem/DirectoryWatcher.h"
#include "Tools/Actions/DeferredActions.h"
#include "Tools/UI/ImGui/imfilebrowser.h"
#include "Tools/UI/ImGui/ImGuiX.h"
#include "Tools/UI/ImGui/Widgets/TileViewWidget.h"
#include "Tools/UI/ImGui/Widgets/TreeListView.h"

namespace Lumina
{
    class CObjectRedirector;
    struct FAssetData;
}


namespace Lumina
{
    class FContentBrowserEditorTool : public FEditorTool
    {
    public:

        struct FPendingOSDrop
        {
            FFixedString Path;
            ImVec2 MousePos;
        };

        struct FPendingRename
        {
            FString OldName;
            FString NewName;
        };

        struct FPendingDestroy
        {
            FFixedString PendingDestroy;
        };

        struct FContentBrowserListViewItemData
        {
            FFixedString Path;
        };

        class FContentBrowserTileViewItem : public FTileViewItem
        {
        public:
            
            FContentBrowserTileViewItem(FTileViewItem* InParent, const FFixedString& InPath)
                : FTileViewItem(InParent)
                , Path(InPath)
            {
            }

            constexpr static const char* DragDropID = "ContentBrowserItem";
            
            void SetDragDropPayloadData() const override
            {
                uintptr_t IntPtr = reinterpret_cast<uintptr_t>(this);
                ImGui::SetDragDropPayload(DragDropID, &IntPtr, sizeof(uintptr_t));
            }

            void DrawTooltip() const override
            {
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(50, 200, 50, 255));
                ImGuiX::TextUnformatted(FileSystem::FileName(Path));
                ImGui::PopStyleColor();

                if (IsAsset())
                {
                    if (const FAssetData* Asset = FAssetQuery().WithPath(Path).ExecuteFirst())
                    {
						ImGuiX::Text(LE_ICON_FILE " Asset Type: {0}", Asset->AssetClass);
                    }
                }
                else if (IsLuaScript())
                {
                    ImGui::Text(LE_ICON_SCRIPT_TEXT " Lua Script");
                }
                else
                {
                    ImGui::TextUnformatted(LE_ICON_FOLDER " Directory");
                }
                
                ImGui::Separator();

                ImGui::Text(LE_ICON_FOLDER " %s", Path.c_str());

                if (!IsDirectory())
                {
                    if (std::filesystem::exists(Path.c_str()))
                    {
                        uint64 Size = std::filesystem::file_size(Path.c_str());
                        ImGuiX::Text(LE_ICON_FILE_CODE " Size: {0}", ImGuiX::FormatSize(Size).c_str());
                    }
                }
            }
            
            bool HasContextMenu() override { return true; }

            NODISCARD FStringView GetName() const override
            {
                return FileSystem::FileName(Path);
            }
            
            void SetPath(FStringView NewPath) { Path = FFixedString{NewPath.begin(), NewPath.end()}; }
            NODISCARD const FFixedString& GetPath() const { return Path; }
            NODISCARD bool IsAsset() const { return FileSystem::IsLuminaAsset(Path); }
            NODISCARD bool IsDirectory() const { return !IsAsset() && !IsLuaScript(); }
            NODISCARD bool IsLuaScript() const { return FileSystem::IsLuaAsset(Path); }
            NODISCARD FStringView GetExtension() const { return FileSystem::Extension(Path); }
            
        private:
            FFixedString                 Path;
        };

        LUMINA_SINGLETON_EDITOR_TOOL(FContentBrowserEditorTool)

        FContentBrowserEditorTool(IEditorToolContext* Context)
            : FEditorTool(Context, "Content Browser", nullptr)
            , ContentBrowserTileView()
        {
        }
        
        bool OnEvent(FEvent& Event) override;
        
        void RefreshContentBrowser();
        bool IsSingleWindowTool() const override { return true; }
        const char* GetTitlebarIcon() const override { return LE_ICON_FORMAT_LIST_BULLETED_TYPE; }
        void OnInitialize() override;
        void OnDeinitialize(const FUpdateContext& UpdateContext) override { }

        void Update(const FUpdateContext& UpdateContext) override;
        void EndFrame() override;

        void InitializeDockingLayout(ImGuiID InDockspaceID, const ImVec2& InDockspaceSize) const override;

        void DrawToolMenu(const FUpdateContext& UpdateContext) override;

        void HandleContentBrowserDragDrop(FStringView DropPath, FStringView PayloadPath);
        
    private:

        void OpenDeletionWarningPopup(const FContentBrowserTileViewItem* Item, const TFunction<void(EYesNo)>& Callback = TFunction<void(EYesNo)>());
        void OnProjectLoaded();

        void TryImport(const FFixedString& Path);
        
        void PushRenameModal(FContentBrowserTileViewItem* ContentItem);
        
        void DrawDirectoryBrowser(bool bIsFocused, ImVec2 Size);
        void DrawContentBrowser(bool bIsFocused, ImVec2 Size);

        void DrawDirectoryContextMenu(FContentBrowserTileViewItem* ContentItem);
        void DrawLuaScriptContextMenu(FContentBrowserTileViewItem* ContentItem);
        void DrawAssetContextMenu(FContentBrowserTileViewItem* ContentItem);
        
        void DrawScriptsDirectoryContextMenu();
        void DrawContentDirectoryContextMenu();

        FDeferredActionRegistry     ActionRegistry;
        FDirectoryWatcher           Watcher;
        
        FTreeListView               DirectoryListView;
        FTreeListViewContext        DirectoryContext;

        FTileViewWidget             ContentBrowserTileView;
        FTileViewContext            ContentBrowserTileViewContext;

        FFixedString                SelectedPath;
        THashMap<FName, bool>       FilterState;
    };
}
