#include "pch.h"
#include "Thread.h"
#include <new>
#include "rpmalloc.h"
#include "Core/Assertions/Assert.h"


namespace Lumina
{
    namespace Threading
    {

        constexpr size_t GCacheLineSize = std::hardware_destructive_interference_size;
        
        static std::thread::id GMainThreadID = {};

        void ThreadYield()
        {
            std::this_thread::yield();
        }

        uint64 GetThreadID()
        {
            return std::this_thread::get_id()._Get_underlying_id();
        }

        bool IsMainThread()
        {
            return GMainThreadID == std::this_thread::get_id();
        }

        uint32 GetNumThreads()
        {
            return std::thread::hardware_concurrency();
        }

        void Sleep(uint64 Milliseconds)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(Milliseconds));
        }

        void Initialize(const char* MainThreadName)
        {
            GMainThreadID = std::this_thread::get_id();
            SetThreadName(MainThreadName);
        }

        void Shutdown()
        {
            GMainThreadID = {};
        }

        void InitializeThreadHeap()
        {
            rpmalloc_thread_initialize();
        }

        void ShutdownThreadHeap()
        {
            rpmalloc_thread_finalize(1);
        }

    }
}
