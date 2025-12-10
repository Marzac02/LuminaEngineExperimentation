#pragma once

#include "ObjectBase.h"
#include "ObjectHandle.h"
#include "Containers/Array.h"
#include "Core/Threading/Atomic.h"
#include "Core/Threading/Thread.h"
#include "Platform/GenericPlatform.h"

namespace Lumina
{
    class CObjectBase;
}

namespace Lumina
{
    struct FCObjectEntry
    {
        FCObjectEntry() = default;
        
        CObjectBase* Object = nullptr;
        TAtomic<int32> Generation{0};
        TAtomic<int32> StrongRefCount{0};
        TAtomic<int32> WeakRefCount{0};
        
        // Non-copyable
        FCObjectEntry(FCObjectEntry&&) = delete;
        FCObjectEntry(const FCObjectEntry&) = delete;
        FCObjectEntry& operator=(FCObjectEntry&&) = delete;
        FCObjectEntry& operator=(const FCObjectEntry&) = delete;

        FORCEINLINE CObjectBase* GetObj() const
        {
            return Object;
        }

        FORCEINLINE void SetObj(CObjectBase* InObject)
        {
            Object = InObject;
        }

        void AddStrongRef()
        {
            StrongRefCount.fetch_add(1, eastl::memory_order_relaxed);
        }

        uint32 ReleaseStrongRef()
        {
            uint32 PrevCount = StrongRefCount.fetch_sub(1, eastl::memory_order_acq_rel);
            return PrevCount - 1;
        }

        void AddWeakRef()
        {
            WeakRefCount.fetch_add(1, eastl::memory_order_relaxed);
        }

        void ReleaseWeakRef()
        {
            WeakRefCount.fetch_sub(1, eastl::memory_order_relaxed);
        }

        int32 GetStrongRefCount() const
        {
            return StrongRefCount.load(eastl::memory_order_relaxed);
        }

        int32 GetWeakRefCount() const
        {
            return WeakRefCount.load(eastl::memory_order_relaxed);
        }

        int32 GetGeneration() const
        {
            return Generation.load(eastl::memory_order_acquire);
        }

        void IncrementGeneration()
        {
            Generation.fetch_add(1, eastl::memory_order_release);
        }

        bool IsReferenced() const
        {
            return StrongRefCount.load(eastl::memory_order_relaxed) > 0;
        }
    };

    /**
     * Global control block for all CObjects, never reallocates.
     */
    class FChunkedFixedCObjectArray
    {
    public:
        
        static constexpr int32 NumElementsPerChunk = 64 * 1024;
    
    private:
        
        // Array of chunk pointers.
        FCObjectEntry** Objects = nullptr;
    
        int32 MaxElements = 0;
        int32 NumElements = 0;
        int32 MaxChunks = 0;
        int32 NumChunks = 0;
    
        FMutex AllocationMutex;
    
    public:
        
        FChunkedFixedCObjectArray()
        {
        }
    
        ~FChunkedFixedCObjectArray()
        {
            Shutdown();
        }

        
        void Initialize(int32 InMaxElements)
        {
            LUM_ASSERT(Objects == nullptr && "Already initialized!")
            LUM_ASSERT(InMaxElements > 0)
    
            MaxElements = InMaxElements;
            MaxChunks = (InMaxElements + NumElementsPerChunk - 1) / NumElementsPerChunk;
            NumChunks = 0;
            NumElements = 0;
    
            Objects = Memory::NewArray<FCObjectEntry*>(MaxChunks);
            Memory::Memzero(Objects, MaxChunks * sizeof(FCObjectEntry*));  // NOLINT(bugprone-multi-level-implicit-pointer-conversion)
            
            PreAllocateAllChunks();
        }
        
        void Shutdown()
        {
            if (Objects)
            {
                // Free all allocated chunks
                for (int32 i = 0; i < NumChunks; ++i)
                {
                    if (Objects[i])
                    {
                        Memory::Delete(Objects[i]);
                        Objects[i] = nullptr;
                    }
                }
    
                Memory::DeleteArray(Objects);
                Objects = nullptr;
            }
    
            MaxElements = 0;
            NumElements = 0;
            MaxChunks = 0;
            NumChunks = 0;
        }
    
        void PreAllocateAllChunks()
        {
            FScopeLock Lock(AllocationMutex);
    
            for (int32 ChunkIndex = 0; ChunkIndex < MaxChunks; ++ChunkIndex)
            {
                if (!Objects[ChunkIndex])
                {
                    Objects[ChunkIndex] = Memory::NewArray<FCObjectEntry>(NumElementsPerChunk);
                    NumChunks = ChunkIndex + 1;
                }
            }
        }
    
        const FCObjectEntry* GetItem(int32 Index) const
        {
            if (Index < 0 || Index >= MaxElements)
            {
                return nullptr;
            }
    
            const int32 ChunkIndex = Index / NumElementsPerChunk;
            const int32 SubIndex = Index % NumElementsPerChunk;
    
            if (ChunkIndex >= NumChunks || !Objects[ChunkIndex])
            {
                return nullptr;
            }
    
            return &Objects[ChunkIndex][SubIndex];
        }
    
        FCObjectEntry* GetItem(int32 Index)
        {
            if (Index < 0 || Index >= MaxElements)
            {
                return nullptr;
            }
    
            const int32 ChunkIndex = Index / NumElementsPerChunk;
            const int32 SubIndex = Index % NumElementsPerChunk;
            
            if (ChunkIndex >= NumChunks || !Objects[ChunkIndex])
            {
                return nullptr;
            }
            
            return &Objects[ChunkIndex][SubIndex];
        }
    
        int32 GetMaxElements() const { return MaxElements; }
        int32 GetNumElements() const { return NumElements; }
        int32 GetNumChunks() const { return NumChunks; }
    
        // Increment element count (called by FCObjectArray on allocation)
        void IncrementElementCount()
        {
            ++NumElements;
        }
    
        // Decrement element count (called by FCObjectArray on deallocation)
        void DecrementElementCount()
        {
            --NumElements;
        }
    
    private:
    };
        
    class FCObjectArray
    {
    private:
        
        FChunkedFixedCObjectArray ChunkedArray;
        
        TVector<int32> FreeIndices;
        
        bool bInitialized = false;
        bool bShuttingDown = false;
    
    public:
        FCObjectArray() = default;
        ~FCObjectArray() = default;
    
        // Initialize the object array with maximum capacity
        void AllocateObjectPool(int32 InMaxCObjects)
        {
            LUM_ASSERT(!bInitialized && "Object pool already allocated!")
    
            ChunkedArray.Initialize(InMaxCObjects);
            
            FreeIndices.reserve(InMaxCObjects / 4);
    
            bInitialized = true;
        }
    
        void Shutdown()
        {
            bShuttingDown = true;
            
            ForEachObject([](CObjectBase* Object, int32)
            {
                Object->OnDestroy();
                Memory::Delete(Object);
            });
            
            ChunkedArray.Shutdown();
            FreeIndices.clear();
            bInitialized = false;
        }
    
        // Allocate a slot for an object and return a handle
        FObjectHandle AllocateObject(CObjectBase* Object)
        {
            LUM_ASSERT(bInitialized && "Object pool not initialized!")
            LUM_ASSERT(Object != nullptr)
            
            int32 Index;
            int32 Generation;
    
            // Try to reuse a free slot
            if (!FreeIndices.empty())
            {
                Index = FreeIndices.back();
                FreeIndices.pop_back();
    
                FCObjectEntry* Item = ChunkedArray.GetItem(Index);
                LUM_ASSERT(Item != nullptr)
                LUM_ASSERT(Item->GetObj() == nullptr)
    
                // Increment generation for reused slot
                Item->IncrementGeneration();
                Generation = Item->GetGeneration();
                
                Item->SetObj(Object);
            }
            else
            {
                // Allocate new slot
                Index = ChunkedArray.GetNumElements();
    
                if (Index >= ChunkedArray.GetMaxElements())
                {
                    LUM_ASSERT(false && "Object pool capacity exceeded!")
                }
    
                FCObjectEntry* Item = ChunkedArray.GetItem(Index);
                LUM_ASSERT(Item != nullptr)
    
                Generation = 1;
                Item->Generation.store(Generation, eastl::memory_order_release);
                Item->SetObj(Object);
    
                ChunkedArray.IncrementElementCount();
            }
    

            return FObjectHandle(Index, Generation);
        }
    
        // Deallocate an object slot
        void DeallocateObject(int32 Index)
        {
            LUM_ASSERT(bInitialized && "Object pool not initialized!")
            
            FCObjectEntry* Item = ChunkedArray.GetItem(Index);
            LUM_ASSERT(Item != nullptr)
            LUM_ASSERT(Item->GetObj() != nullptr)
    
            Item->SetObj(nullptr);
    
            Item->IncrementGeneration();
    
            FreeIndices.push_back(Index);
    
        }
    
        // Resolve a handle to an object pointer
        LUMINA_DISABLE_OPTIMIZATION
        CObjectBase* ResolveHandle(const FObjectHandle& Handle) const
        {
            if (!Handle.IsValid())
            {
                return nullptr;
            }
    
            const FCObjectEntry* Item = ChunkedArray.GetItem(Handle.Index);
            if (!Item)
            {
                return nullptr;
            }
    
            const int32 Generation = Item->GetGeneration();
            if (Generation != Handle.Generation)
            {
                return nullptr;
            }
    
            return Item->GetObj();
        }
        LUMINA_ENABLE_OPTIMIZATION
        CObjectBase* GetObjectByIndex(int32 Index) const
        {
            const FCObjectEntry* Item = ChunkedArray.GetItem(Index);
            return Item ? Item->GetObj() : nullptr;
        }

        FObjectHandle GetHandleByObject(const CObjectBase* Object) const
        {
            return GetHandleByIndex(Object->GetInternalIndex());
        }
        
    
        // Get handle from object index and current generation
        FObjectHandle GetHandleByIndex(int32 Index) const
        {
            const FCObjectEntry* Item = ChunkedArray.GetItem(Index);
            if (!Item || !Item->GetObj())
            {
                return FObjectHandle();
            }
    
            return FObjectHandle(Index, Item->GetGeneration());
        }

        // Add a strong reference to an object by pointer
        void AddStrongRef(CObjectBase* Object)
        {
            if (Object)
            {
                FCObjectEntry* Item = ChunkedArray.GetItem(Object->GetInternalIndex());
                if (Item)
                {
                    Item->AddStrongRef();
                }
            }
        }

        // Release a strong reference by pointer
        // Returns true if object was deleted.
        bool ReleaseStrongRef(CObjectBase* Object)
        {
            // Shutting down means we're manually destroying every object, so we don't want to do anything else.
            if (bShuttingDown)
            {
                return false;
            }
            
            if (Object)
            {
                FCObjectEntry* Item = ChunkedArray.GetItem(Object->GetInternalIndex());
                if (Item)
                {
                    uint32 NewCount = Item->ReleaseStrongRef();
                    if (NewCount == 0)
                    {
                        Object->ConditionalBeginDestroy();
                        return true;
                    }
                }
            }
            return false;
        }
    
        void AddStrongRefByIndex(int32 Index)
        {
            FCObjectEntry* Item = ChunkedArray.GetItem(Index);
            if (Item)
            {
                Item->AddStrongRef();
            }
        }
    
        bool ReleaseStrongRefByIndex(int32 Index)
        {
            FCObjectEntry* Item = ChunkedArray.GetItem(Index);
            if (Item)
            {
                uint32 NewCount = Item->ReleaseStrongRef();
                return NewCount == 0 && Item->GetObj() != nullptr;
            }
            return false;
        }

        void AddWeakRefByIndex(int32 Index)
        {
            FCObjectEntry* Item = ChunkedArray.GetItem(Index);
            if (Item)
            {
                Item->AddWeakRef();
            }
        }

        void ReleaseWeakRefByIndex(int32 Index)
        {
            FCObjectEntry* Item = ChunkedArray.GetItem(Index);
            if (Item)
            {
                Item->ReleaseWeakRef();
            }
        }

        bool IsReferencedByIndex(int32 Index) const
        {
            const FCObjectEntry* Item = ChunkedArray.GetItem(Index);
            return Item && Item->IsReferenced();
        }

        int32 GetStrongRefCountByIndex(int32 Index) const
        {
            const FCObjectEntry* Item = ChunkedArray.GetItem(Index);
            return Item ? Item->GetStrongRefCount() : 0;
        }
    
        int32 GetNumAliveObjects() const
        {
            return ChunkedArray.GetNumElements() - (int32)FreeIndices.size();
        }
    
        int32 GetMaxObjects() const
        {
            return ChunkedArray.GetMaxElements();
        }
    
        template<typename Func>
        requires(eastl::is_invocable_v<Func, CObjectBase*, int32>)
        void ForEachObject(Func&& Function) const
        {
            const int32 MaxElements = ChunkedArray.GetNumElements();
            
            for (int32 i = 0; i < MaxElements; ++i)
            {
                const FCObjectEntry* Item = ChunkedArray.GetItem(i);
                if (Item && Item->GetObj())
                {
                    std::forward<Func>(Function)(Item->GetObj(), i);
                }
            }
        }
    };
    
    // Global object array instance
    extern LUMINA_API FCObjectArray GObjectArray;
    
}
