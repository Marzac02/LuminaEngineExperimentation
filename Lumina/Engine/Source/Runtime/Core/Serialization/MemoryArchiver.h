#pragma once

#include "Archiver.h"
#include "Containers/Array.h"
#include "Core/Assertions/Assert.h"
#include "Memory/Memcpy.h"
#include "Memory/Memory.h"

namespace Lumina
{

    class FMemoryArchiver : public FArchive
    {
    public:

        void Seek(int64 InPos) final
        {
            Offset = InPos;
        }
        
        int64 Tell() final
        {
            return Offset;
        }
        
    protected:

        FMemoryArchiver()
            : FArchive()
            , Offset(0)
        {}
        
        int64 Offset;
    };
    
    class FMemoryReader : public FMemoryArchiver
    {
    public:

        explicit FMemoryReader(const TVector<uint8>& InBytes, bool bIsPersistent = false);
        int64 TotalSize() override;
        void SetLimitSize(int64 NewLimitSize);

        void Serialize(void* V, int64 Size) override;

    private:

        const TVector<uint8>&   Bytes;
        int64                   LimitSize;
    
    };

    /**
     * Similar to FMemoryReader but owns the data.
     */
    class FBufferReader : public FArchive
    {
    public:
        FBufferReader(void* InData, int64 InSize, bool bFreeAfterClose);
        ~FBufferReader() override;
        
        
        int64 Tell() final;
        int64 TotalSize() final;
        void Seek(int64 InPos) final;
        void Serialize(void* Data, int64 Size) override;

    private:
        
        void*   ReaderData;
        int64   ReaderPos;
        int64   ReaderSize;
        bool    bFreeOnClose = false;
    };

        
    class FMemoryWriter : public FMemoryArchiver
    {
    public:

        FMemoryWriter(TVector<uint8>& InBytes, bool bSetOffset = false);
        int64 TotalSize() override { return static_cast<int64>(Bytes.size()); }
        void Serialize(void* Data, int64 Size) override;
        
    private:
        TVector<uint8>&	Bytes;
    };
}
