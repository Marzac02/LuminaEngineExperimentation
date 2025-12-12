#pragma once
#include "EditorTool.h"
#include "Assets/AssetRegistry/AssetData.h"
#include "Core/LuminaCommonTypes.h"
#include "Core/Object/ObjectRename.h"
#include "Core/Object/Package/Package.h"
#include "Paths/Paths.h"
#include "Platform/Filesystem/DirectoryWatcher.h"
#include "Renderer/RHIFwd.h"
#include "Tools/UI/ImGui/imfilebrowser.h"
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
            FString Path;
            ImVec2 MousePos;
        };

        struct FPendingRename
        {
            FString OldName;
            FString NewName;
        };

        struct FPendingDestroy
        {
            FString PendingDestroy;
        };

        class FContentBrowserListViewItem : public FTreeListViewItem
        {
        public:

            FContentBrowserListViewItem(FTreeListViewItem* InParent, const FString& InPath, const FString& InDisplayName)
                : FTreeListViewItem(InParent)
                , DisplayName(InDisplayName)
                , Path(InPath)
            {}
            
            virtual ~FContentBrowserListViewItem() override { }

            const char* GetTooltipText() const override { return Path.c_str(); }
            bool HasContextMenu() override { return true; }
            
            
            FInlineString GetDisplayName() const override
            {
                return FInlineString()
                .append(LE_ICON_FOLDER)
                .append(" ")
                .append(GetName().c_str());
            }

            FName GetName() const override
            {
                return DisplayName.c_str();
            }

            uint64 GetHash() const override { return Hash::GetHash64(Path); }

            const FString& GetPath() const { return Path; }
            
        private:

            FString DisplayName;
            FString Path;
        };

        class FContentBrowserTileViewItem : public FTileViewItem
        {
        public:
            
            FContentBrowserTileViewItem(FTileViewItem* InParent, const FString& InPath)
                : FTileViewItem(InParent)
                , Path(InPath)
                , VirtualPath(Paths::ConvertToVirtualPath(InPath))
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
                ImGui::TextUnformatted(Paths::FileName(VirtualPath).c_str());
                ImGui::PopStyleColor();

                if (IsAsset())
                {
                    ImGui::Text(LE_ICON_FILE " Lumina Asset");
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

                ImGui::Text(LE_ICON_FOLDER " %s", VirtualPath.c_str());

                if (!IsDirectory())
                {
                    if (std::filesystem::exists(Path.c_str()))
                    {
                        uint64 Size = std::filesystem::file_size(Path.c_str());
                        double SizeKiB = static_cast<double>(Size) / 1024.0;
                        ImGui::Text(LE_ICON_FILE_CODE " Size: %.2f KiB", SizeKiB);
                    }
                }
            }
            
            bool HasContextMenu() override { return true; }

            FName GetName() const override
            {
                FInlineString NameString;
                NameString.append(Paths::FileName(Path, true).c_str());
                return NameString;
            }

            FString GetFileName() const { return Paths::FileName(Path, true); }

            void SetPath(FStringView NewPath) { Path = NewPath; Paths::ConvertToVirtualPath(Path); }
            const FString& GetPath() const { return Path; }
            const FString& GetVirtualPath() const { return VirtualPath; }
            bool IsAsset() const { return Paths::GetExtension(Path) == "lasset"; }
            bool IsDirectory() const { return !IsAsset() && !IsLuaScript(); }
            bool IsLuaScript() const { return Paths::GetExtension(Path) == "lua"; }
            
        private:
            
            FString                 Path;
            FString                 VirtualPath;
        };

        LUMINA_SINGLETON_EDITOR_TOOL(FContentBrowserEditorTool)

        FContentBrowserEditorTool(IEditorToolContext* Context)
            : FEditorTool(Context, "Content Browser", nullptr)
            , OutlinerContext()
            , ContentBrowserTileView()
            , ContentBrowserTileViewContext()
        {
        }

        using FPendingActionsVector = TTupleVector<FPendingDestroy, FPendingOSDrop, FPendingRename>;
        
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

        void HandleContentBrowserDragDrop(FContentBrowserTileViewItem* Drop, FContentBrowserTileViewItem* Payload);
        
    private:

        void OpenDeletionWarningPopup(const FContentBrowserTileViewItem* Item, const TFunction<void(EYesNo)>& Callback = TFunction<void(EYesNo)>());
        void OnProjectLoaded();

        void TryImport(const FString& Path);
        
        ObjectRename::EObjectRenameResult HandleRenameEvent(const FString& OldPath, const FString& NewPath);

        void PushRenameModal(FContentBrowserTileViewItem* ContentItem);
        
        void DrawDirectoryBrowser(const FUpdateContext& Context, bool bIsFocused, ImVec2 Size);
        void DrawContentBrowser(const FUpdateContext& Context, bool bIsFocused, ImVec2 Size);

        void DrawDirectoryContextMenu(FContentBrowserTileViewItem* ContentItem);
        void DrawLuaScriptContextMenu(FContentBrowserTileViewItem* ContentItem);
        void DrawAssetContextMenu(FContentBrowserTileViewItem* ContentItem);
        
        void DrawScriptsDirectoryContextMenu();
        void DrawContentDirectoryContextMenu();

        template<typename T, typename ... TArgs>
        T& EmplaceAction(TArgs&& ... Args)
        {
            entt::entity NewActionEntity = PendingActions.create();
            return PendingActions.emplace<T>(NewActionEntity, Forward<TArgs>(Args)...);
        }

        entt::registry              PendingActions;
        
        FDirectoryWatcher           Watcher;
        FTreeListView               OutlinerListView;
        FTreeListViewContext        OutlinerContext;

        FTileViewWidget             ContentBrowserTileView;
        FTileViewContext            ContentBrowserTileViewContext;

        FString                     SelectedPath;
        THashMap<FName, bool>       FilterState;
    };
}
