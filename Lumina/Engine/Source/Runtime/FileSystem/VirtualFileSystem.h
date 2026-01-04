#pragma once
#include "Containers/Array.h"
#include "Containers/Name.h"
#include "FileSystem.h"
#include "Core/Templates/LuminaTemplate.h"
#include "Memory/SmartPtr.h"

namespace Lumina::FileSystem
{
    using FileSystemMap = THashMap<FName, TUniquePtr<IFileSystem>>;
    
    class FVirtualFileSystem
    {
    public:
        
        static void Initialize();
        static void Shutdown();
        
        template<typename T, typename ... TArgs>
        requires(eastl::is_constructible_v<T, FName, TArgs...> && eastl::is_base_of_v<IFileSystem, T>)
        T* CreateFileSystem(const FName& Alias, TArgs&&... Args)
        {
            auto [It, bInserted] = Systems.try_emplace(Alias, MakeUniquePtr<T>(Alias, Forward<TArgs>(Args)...));
            return static_cast<T*>(It->second.get());
        }
    
        template<typename T>
        T& GetFileSystem(const FName& Alias)
        {
            return *Systems.at(Alias);
        }
        
        FORCEINLINE bool DoesAliasExists(const FName& Alias) const;
        
    private:
        
        FileSystemMap Systems;
    };
}

namespace Lumina
{
    LUMINA_API extern FileSystem::FVirtualFileSystem* GFileSystem; 
}
