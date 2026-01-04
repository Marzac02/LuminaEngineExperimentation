#include "pch.h"
#include "VirtualFileSystem.h"

namespace Lumina
{
    LUMINA_API FileSystem::FVirtualFileSystem* GFileSystem = nullptr;
}
namespace Lumina::FileSystem
{
    
    void FVirtualFileSystem::Initialize()
    {
        GFileSystem = Memory::New<FVirtualFileSystem>();
    }

    void FVirtualFileSystem::Shutdown()
    {
        Memory::Delete(GFileSystem);
        GFileSystem = nullptr;
    }

    bool FVirtualFileSystem::DoesAliasExists(const FName& Alias) const
    {
        return Systems.count(Alias);
    }
}
