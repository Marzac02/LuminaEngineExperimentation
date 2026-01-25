#pragma once
#include "EASTL/vector.h"
#include "Reflector/Types/StructReflectItem.h"
#include "Reflector/Utils/MetadataUtils.h"

namespace Lumina
{
    class FReflectedFunction : public IStructReflectable
    {
    public:

        FReflectedFunction() = default;
        
        void GenerateMetadata(const eastl::string& InMetadata) override;
        
        eastl::vector<FMetadataPair>    Metadata;
        eastl::string                   Name;
        eastl::string                   Outer;    
    };
}
