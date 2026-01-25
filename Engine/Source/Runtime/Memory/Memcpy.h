#pragma once

namespace Lumina::Memory
{
	FORCEINLINE void Memcpy(void* Destination, void* Source, size_t SrcSize)
	{
		std::memcpy(Destination, Source, SrcSize);
	}
    
	FORCEINLINE void Memcpy(void* Destination, const void* Source, size_t SrcSize)
	{
		std::memcpy(Destination, Source, SrcSize);
	}
}