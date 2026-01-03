#include "pch.h"
#include "Name.h"

#include "Memory/Memory.h"


namespace Lumina
{
    LUMINA_API FNameTable* GNameTable = nullptr;

    const char* FStringPool::AllocateString(const char* Str, size_t Length)
    {
        size_t AlignedLength = (Length + 1 + 7) & ~7;
            
        if (!Current || Current->Used + AlignedLength > CHUNK_SIZE)
        {
            Current = Memory::New<Chunk>();
            Current->Next = Head;
            Head = Current;
        }
            
        char* Result = Current->Data + Current->Used;
        memcpy(Result, Str, Length);
        Result[Length] = '\0';
        Current->Used += AlignedLength;
            
        return Result;
    }

    FStringPool::~FStringPool()
    {
        while (Head)
        {
            Chunk* Next = Head->Next;
            Memory::Delete(Head);
            Head = Next;
        }
    }

    void FName::Initialize()
    {
        GNameTable = Memory::New<FNameTable>();
        std::cout << "[Lumina] - String ID (FName) System Initialized\n";
        std::cout.flush();
    }

    void FName::Shutdown()
    {
        Memory::Delete(GNameTable);
        GNameTable = nullptr;
    }

    char FName::At(size_t Pos) const
    {
        const char* Str = c_str();
        size_t Len = strlen(Str);
    
        if (Pos >= Len)
        {
            return '\0';
        }
    
        return Str[Pos];
    }
}
