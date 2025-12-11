#pragma once
#include "Core/Object/ConstructObjectParams.h"
#include "Module/API.h"

namespace Lumina
{
    class CObject;
    class CPackage;
}

namespace Lumina
{
    
    class FObjectInitializer
    {
    public:

        LUMINA_API FObjectInitializer()
            : Package(nullptr)
            , Params(nullptr)
        {}
        
        LUMINA_API FObjectInitializer(CPackage* InPackage, const FConstructCObjectParams& InParams);
        ~FObjectInitializer();

        static FObjectInitializer* Get();

        void Construct();

        CPackage* Package;
        FConstructCObjectParams Params;
    };
    
}
