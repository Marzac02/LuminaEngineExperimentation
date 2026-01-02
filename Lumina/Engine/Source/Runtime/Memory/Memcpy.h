#pragma once

#include "Platform/GenericPlatform.h"
#include "Platform/Platform.h"

namespace Lumina::Memory
{
	FORCEINLINE void Memcpy(void* RESTRICT Destination, void* RESTRICT Source, uint64 SrcSize)
	{
		std::memcpy(Destination, Source, SrcSize);
	}
    
	FORCEINLINE void Memcpy(void* RESTRICT Destination, const void* RESTRICT Source, uint64 SrcSize)
	{
		std::memcpy(Destination, Source, SrcSize);
	}
}