#pragma once
#include "imgui.h"
#include "Containers/Array.h"
#include "Containers/Name.h"
#include "Containers/Function.h"
#include "Memory/Memory.h"
#include "Memory/Allocators/Allocator.h"

namespace Lumina
{

    class LUMINA_API FTreeListViewItem
    {
        friend class FTreeListView;
        
    public:
        
        virtual ~FTreeListViewItem() = default;

        FTreeListViewItem(FTreeListViewItem* InParent)
            : Parent(InParent)
            , bExpanded(false)
            , bVisible(false)
            , bSelected(false)
        {}
        
        virtual FString GetName() const = 0;
        
        virtual const char* GetTooltipText() const { return nullptr; }

        virtual bool HasContextMenu() { return false; }

        virtual ImVec4 GetDisplayColor() const;
        
        virtual void OnSelectionStateChanged() { }

        bool HasChildren() const { return !Children.empty(); }

        virtual void SetDragDropPayloadData() const { }

        virtual uint64 GetHash() const = 0;
        
        virtual FFixedString GetDisplayName() const
        {
            return GetName().c_str();
        }

        template<typename T, typename... Args>
        requires (eastl::is_base_of_v<FTreeListViewItem, T> && eastl::is_constructible_v<T, Args...>)
        T* AddChild(Args&&... args)
        {
            T* New = Allocator->TAlloc<T>(eastl::forward<Args>(args)...);
            New->Allocator = Allocator;
            Children.push_back(New);

            return New;
        }

    private:

        // Disable copies/moves
        FTreeListViewItem& operator=(FTreeListViewItem const&) = delete;
        FTreeListViewItem& operator=(FTreeListViewItem&&) = delete;


    protected:

        FBlockLinearAllocator*                  Allocator = nullptr;
        FTreeListViewItem*                      Parent = nullptr;
        TFixedVector<FTreeListViewItem*, 4>     Children;

        uint8                               bExpanded:1;
        uint8                               bVisible:1;
        uint8                               bSelected:1;
        
    };

    struct LUMINA_API FTreeListViewContext
    {
        /** Check if the item to draw passes a filter */
        TFunction<bool(const FTreeListViewItem&)>                   FilterFunction;
        
        /** Callback to draw any context menus this item may want */
        TFunction<void(const TVector<FTreeListViewItem*>&)>         DrawItemContextMenuFunction;

        /** Called when a rebuild of the widget tree is requested */
        TFunction<void(FTreeListView*)>                             RebuildTreeFunction;

        /** Called when an item has been selected in the tree */
        TFunction<void(FTreeListViewItem*)>                         ItemSelectedFunction;

        /** Called when we have a drag-drop operation on a target */
        TFunction<void(FTreeListViewItem*)>                         DragDropFunction;
        
        /** Called when a key is pressed while hovering the tile item, return true to absorb. */
        TFunction<bool(FTreeListViewItem&, ImGuiKey)>               KeyPressedFunction;
    };
    
    
    class LUMINA_API FTreeListView
    {
    public:

        FTreeListView()
            : Allocator(1024)
            , bMaintainVisibleRowIndex(false)
            , bDirty(false)
            , bCurrentlyDrawing(false)
        {
        }

        ~FTreeListView()
        {
            ClearTree();
        }

        FTreeListView(const FTreeListView&) = delete;
        FTreeListView& operator=(const FTreeListView&) = delete;
        
        void Draw(const FTreeListViewContext& Context);

        void ClearTree();

        void MarkTreeDirty() { bDirty = true; }
        NODISCARD bool IsCurrentlyDrawing() const { return bCurrentlyDrawing; }
        NODISCARD bool IsDirty() const { return bDirty; }
        
        template<typename T, typename... Args>
        requires (std::is_base_of_v<FTreeListViewItem, T> && std::is_constructible_v<T, Args...>)
        T* AddItemToTree(Args&&... args)
        {
            T* New = Allocator.TAlloc<T>(eastl::forward<Args>(args)...);
            New->Allocator = &Allocator;
            ListItems.push_back(New);

            return New;
        }
        
        void SetSelection(FTreeListViewItem* Item, const FTreeListViewContext& Context);
        
    private:

        bool HandleKeyPressed(const FTreeListViewContext& Context, FTreeListViewItem& Item, ImGuiKey Key);

        
        void RequestScrollTo(const FTreeListViewItem* Item);
        
        void RebuildTree(const FTreeListViewContext& Context);
        
        void DrawListItem(FTreeListViewItem* ItemToDraw, const FTreeListViewContext& Context);

        void ClearSelection();

        void ForEachItem(const TFunction<void(FTreeListViewItem* Item)>& Functor);

    private:
        
        FBlockLinearAllocator                   Allocator;
        
        TVector<FTreeListViewItem*>             Selections;
        TVector<FTreeListViewItem*>             ListItems;

        float                                   EstimatedRowHeight = -1.0f;
        int32                                   FirstVisibleRowItemIndex = 0;
        
        uint8                                   bMaintainVisibleRowIndex:1;
        uint8                                   bDirty:1;
        uint8                                   bCurrentlyDrawing:1;
    };
}