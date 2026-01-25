#pragma once
#include "VulkanResources.h"
#include "Platform/GenericPlatform.h"

namespace Lumina
{
    enum class EResourceStates : unsigned int;
    enum class FVulkanImage::ESubresourceViewType : uint8;
    struct FResourceStateMapping;
    struct FResourceStateMapping2;
}

namespace Lumina::Vk
{
    FResourceStateMapping ConvertResourceState(EResourceStates State);
    FResourceStateMapping2 ConvertResourceState2(EResourceStates State);

    FVulkanImage::ESubresourceViewType GetTextureViewType(EFormat BindingFormat, EFormat TextureFormat);

}
