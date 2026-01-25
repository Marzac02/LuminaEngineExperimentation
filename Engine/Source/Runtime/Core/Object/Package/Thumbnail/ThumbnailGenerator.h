#pragma once

namespace Lumina
{
    class FRHIImage;
    class CObject;
}

namespace Lumina::ThumbnailGenerator
{
    RUNTIME_API FRHIImage* GenerateImageForObject(CObject* Object);
 
}
