#include "pch.h"
#include "Memory.h"
#include "Core/Profiler/Profile.h"

//#undef LUMINA_RPMALLOC

namespace Lumina
{
    static void CustomAssert(const char* pMessage)
    {
        if (pMessage && strstr(pMessage, "Memory leak detected"))
        {
            return;
        }
        
        PANIC("{}", pMessage);
    }
    
    void Memory::Initialize()
    {
        LUMINA_PROFILE_SCOPE();

        if (!GIsMemorySystemInitialized)
        {
#if defined(LUMINA_RPMALLOC)

            Memzero(&GrpmallocConfig, sizeof(rpmalloc_config_t));
            GrpmallocConfig.error_callback = CustomAssert;
            GrpmallocConfig.enable_huge_pages = true;
            
        
            rpmalloc_initialize_config(&GrpmallocConfig);

            GIsMemorySystemInitialized = true;
#endif
            std::cout << "[Lumina] - Memory System Initialized\n";
            std::cout.flush();
        }
    }
    
    void Memory::Shutdown()
    {
        LUMINA_PROFILE_SCOPE();
#if defined(LUMINA_RPMALLOC)

        rpmalloc_global_statistics_t stats;
        rpmalloc_global_statistics(&stats);
        
        std::cout << "[Lumina] - Memory System Shutdown.\n";

        rpmalloc_finalize();
#endif
        GIsMemorySystemInitialized = false;
        GIsMemorySystemShutdown = true;
    }

    void Memory::InitializeThreadHeap()
    {
#if defined(LUMINA_RPMALLOC)
        rpmalloc_thread_initialize();
#endif
    }

    SIZE_T Memory::GetActualAlignment(size_t Alignment)
    {
        SIZE_T Align = (Alignment < 16) ? 16 : (Alignment);
        return Align;
    }

    void* Memory::Malloc(size_t size, size_t alignment)
    {
#if defined(LUMINA_RPMALLOC)
        if(!GIsMemorySystemInitialized)
        {
            Initialize();
        }
        
        SIZE_T ActualAlignment = GetActualAlignment(alignment);
        void* pMemory = rpaligned_alloc(ActualAlignment, size);
        
#else
        SIZE_T ActualAlignment = GetActualAlignment(alignment);
        void* pMemory = _aligned_malloc(size, ActualAlignment);
#endif
        LUMINA_PROFILE_ALLOC(pMemory, size);

        return pMemory;
    }

    void* Memory::Realloc(void* Memory, size_t NewSize, size_t OriginalAlignment)
    {
        SIZE_T ActualAlignment = GetActualAlignment(OriginalAlignment);
        
#if LUMINA_RPMALLOC
        void* pReallocatedMemory = rpaligned_realloc(Memory, ActualAlignment, NewSize, 0, 0);

#else
        void* pReallocatedMemory = _aligned_realloc(Memory, NewSize, ActualAlignment);
#endif
        
        return pReallocatedMemory;
        
    }

    void Memory::Free(void*& Memory)
    {
#if defined(LUMINA_RPMALLOC)
        LUMINA_PROFILE_FREE(Memory);
        rpfree(Memory);
        Memory = nullptr;
#else
        LUMINA_PROFILE_FREE(Memory);
        _aligned_free(Memory);
        Memory = nullptr;
#endif
    }
}
