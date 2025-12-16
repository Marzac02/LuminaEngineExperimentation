#pragma once

#define USE_IMGUI_API
#include "EditorTool.h"
#include "Core/Delegates/Delegate.h"
#include "Tools/UI/ImGui/ImGuiFonts.h"
#include <imgui.h>
#include "Tools/UI/ImGui/ImGuizmo.h"
#include "Tools/UI/ImGui/Widgets/TreeListView.h"
#include "UI/Properties/PropertyTable.h"
#include "World/Entity/Components/NameComponent.h"
#include "World/Entity/Systems/EntitySystem.h"

namespace Lumina
{
    DECLARE_MULTICAST_DELEGATE(FOnGamePreview);
    
    
    /**
     * Base class for display and manipulating scenes.
     */
    class FWorldEditorTool : public FEditorTool
    {
        using Super = FEditorTool;
        LUMINA_SINGLETON_EDITOR_TOOL(FWorldEditorTool)


        class FEntityListViewItem : public FTreeListViewItem
        {
        public:

            FEntityListViewItem(FTreeListViewItem* InParent, FEntityRegistry& InRegistry, entt::entity InEntity)
                : FTreeListViewItem(InParent)
                , Entity(InEntity)
                , Registry(InRegistry)
            {}
            
            ~FEntityListViewItem() override { }

            constexpr static const char* DragDropID = "EntityItem";
            
            const char* GetTooltipText() const override { return GetName().c_str(); }
            bool HasContextMenu() override { return true; }
            uint64 GetHash() const override { return (uint64)Entity; }
            void SetDragDropPayloadData() const override
            {
                uintptr_t IntPtr = (uintptr_t)this;
                ImGui::SetDragDropPayload(DragDropID, &IntPtr, sizeof(uintptr_t));
            }
            
            FName GetName() const override
            {
                SNameComponent* NameComponent = Registry.try_get<SNameComponent>(Entity);
                return NameComponent ? NameComponent->Name : eastl::to_string((uint32)Entity);
            }

            entt::entity GetEntity() const { return Entity; }
            
        private:

            entt::entity Entity;
            FEntityRegistry& Registry;
        };

        

        class FSystemListViewItem : public FTreeListViewItem
        {
        public:

            FSystemListViewItem(FTreeListViewItem* InParent, CEntitySystem* InSystem)
                : FTreeListViewItem(InParent)
                , System(InSystem)
            {
                Hash = System.Lock()->GetName();
                Name = System.Lock()->GetName();
            }

            ~FSystemListViewItem() override { }

            const char* GetTooltipText() const override { return GetName().c_str(); }
            bool HasContextMenu() override { return true; }
            uint64 GetHash() const override { return Hash; }
            FName GetName() const override
            {
                return Name;
            }

            CEntitySystem* GetSystem() const { return System.Lock(); }
            
        private:

            uint64 Hash;
            FName Name;
            TWeakObjectPtr<CEntitySystem> System;
        };

        struct FEntityListFilterState
        {
            ImGuiTextFilter FilterName;
            TVector<FName> ComponentFilters;
        };

        struct FComponentDestroyRequest
        {
            const CStruct* Type;
            entt::entity EntityID;
        };
        
    public:
        
        FWorldEditorTool(IEditorToolContext* Context, CWorld* InWorld);
        ~FWorldEditorTool() noexcept override { }

        void OnInitialize() override;
        void OnDeinitialize(const FUpdateContext& UpdateContext) override;

        void Update(const FUpdateContext& UpdateContext) override;

        void DrawToolMenu(const FUpdateContext& UpdateContext) override;
        void InitializeDockingLayout(ImGuiID InDockspaceID, const ImVec2& InDockspaceSize) const override;

        void DrawViewportOverlayElements(const FUpdateContext& UpdateContext, ImTextureRef ViewportTexture, ImVec2 ViewportSize) override;
        void DrawViewportToolbar(const FUpdateContext& UpdateContext) override;

        void PushAddTagModal(entt::entity Entity);
        void PushAddComponentModal(entt::entity Entity);
        void PushRenameEntityModal(entt::entity Entity);

		void OnSave() override;

        bool IsAssetEditorTool() const override;
        FOnGamePreview& GetOnPreviewStartRequestedDelegate() { return OnGamePreviewStartRequested; }
        FOnGamePreview& GetOnPreviewStopRequestedDelegate() { return OnGamePreviewStopRequested; }

        void NotifyPlayInEditorStart();
        void NotifyPlayInEditorStop();

        void SetWorld(CWorld* InWorld) override;
        
    protected:

        void SetWorldNewSimulate(bool bShouldSimulate);

        void DrawCreateEntityMenu();
        void DrawFilterOptions();
        void SetSelectedEntity(entt::entity Entity);
        void RebuildSceneOutliner(FTreeListView* View);
        void HandleEntityEditorDragDrop(FTreeListViewItem* DropItem);

        void DrawOutliner(const FUpdateContext& UpdateContext, bool bFocused);
        void DrawSystems(const FUpdateContext& UpdateContext, bool bFocused);
        void DrawEntityProperties();
        void DrawEntityActionButtons();
        void DrawComponentList();
        void DrawTagList();
        void DrawComponentHeader(TUniquePtr<FPropertyTable>& Table);
        void RemoveComponent(entt::entity Entity, const CStruct* ComponentType);
        void DrawSystemProperties();
        void DrawEmptyState();

        void DrawObjectEditor(const FUpdateContext& UpdateContext, bool bFocused);

        void DrawPropertyEditor(const FUpdateContext& UpdateContext, bool bFocused);

        void RebuildPropertyTables();

        void CreateEntityWithComponent(const CStruct* Component);
        void CreateEntity();

        void CopyEntity(entt::entity& To, entt::entity From);

        void CycleGuizmoOp();

    private:

        TObjectPtr<CWorld>                      WorldState;
        
        ImGuiTextFilter                         AddEntityComponentFilter;
        FEntityListFilterState                  EntityFilterState;
        FOnGamePreview                          OnGamePreviewStartRequested;
        FOnGamePreview                          OnGamePreviewStopRequested;
        
        ImGuizmo::OPERATION                     GuizmoOp;
        ImGuizmo::MODE                          GuizmoMode;

        CEntitySystem*                          SelectedSystem;
        entt::entity                            SelectedEntity;
        entt::entity                            CopiedEntity;
        
        FTreeListView                           OutlinerListView;
        FTreeListViewContext                    OutlinerContext;

        FTreeListView                           SystemsListView;
        FTreeListViewContext                    SystemsContext;

        TQueue<FComponentDestroyRequest>        ComponentDestroyRequests;
        TQueue<entt::entity>                    EntityDestroyRequests;
        TVector<TUniquePtr<FPropertyTable>>     PropertyTables;

        bool                                    bGamePreviewRunning = false;
        bool                                    bSimulatingWorld = false;

        /** IDK, this thing will return IsUsing = true always if it's never been used */
        bool                                    bImGuizmoUsedOnce = false;
    };
    
}
