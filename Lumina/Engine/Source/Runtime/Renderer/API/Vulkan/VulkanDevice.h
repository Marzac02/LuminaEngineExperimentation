#pragma once

#include <volk/volk.h>
#include "vk_mem_alloc.h"
#include "Containers/Array.h"
#include "Core/LuminaMacros.h"
#include "Core/Threading/Thread.h"
#include "Memory/Memory.h"

namespace Lumina
{
    class FVulkanBuffer;
    class FRHIBuffer;
    class IDeviceChild;
    class FVulkanRenderContext;
}

namespace Lumina
{

    class FVulkanMemoryAllocator
    {
    public:

        struct PoolConfig
        {
            VmaPool Pool = VK_NULL_HANDLE;
            uint32 BlockSize = 0;
        };


        FVulkanMemoryAllocator(FVulkanRenderContext* InCxt, VkInstance Instance, VkPhysicalDevice PhysicalDevice, VkDevice Device);
        ~FVulkanMemoryAllocator();
        
        void GetMemoryBudget(VmaBudget* OutBudgets);
        void LogMemoryStats();
        
        VmaAllocation AllocateBuffer(const VkBufferCreateInfo* CreateInfo, VmaAllocationCreateFlags Flags, VkBuffer* vkBuffer, const char* AllocationName) const;
        VmaAllocation AllocateImage(const VkImageCreateInfo* CreateInfo, VmaAllocationCreateFlags Flags, VkImage* vkImage, const char* AllocationName) const;

        VmaAllocator GetVMA() const { return Allocator; }

        void DestroyBuffer(VkBuffer Buffer, VmaAllocation Allocation) const;
        void DestroyImage(VkImage Image, VmaAllocation Allocation) const;

        /** All buffers created with VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT will have their memory persistently mapped *
         * https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/memory_mapping.html
         * */
        void* GetMappedMemory(const FVulkanBuffer* Buffer) const;

    
    private:
        
        VmaAllocator Allocator = nullptr;
        FVulkanRenderContext* RenderContext = nullptr;
    };
    
    class FVulkanDevice
    {
    public:

        FVulkanDevice(FVulkanRenderContext* RenderContext, VkInstance Instance, VkPhysicalDevice InPhysicalDevice, VkDevice InDevice)
            : PhysicalDevice(InPhysicalDevice)
            , Device(InDevice)
        {
            vkGetPhysicalDeviceMemoryProperties(PhysicalDevice, &PhysicalDeviceMemoryProperties);
            vkGetPhysicalDeviceProperties(PhysicalDevice, &PhysicalDeviceProperties);

            VkPhysicalDeviceFeatures2 features2{};
            features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

            Features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
            Features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
            Features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;

            features2.pNext = &Features11;
            Features11.pNext = &Features12;
            Features12.pNext = &Features13;

            vkGetPhysicalDeviceFeatures2(PhysicalDevice, &features2);

            Features10 = features2.features;
            
            Allocator = Memory::New<FVulkanMemoryAllocator>(RenderContext, Instance, PhysicalDevice, Device);
        }

        virtual ~FVulkanDevice()
        {
            Memory::Delete(Allocator);
            vkDestroyDevice(Device, nullptr);
        }
        
        LE_NO_COPYMOVE(FVulkanDevice);

        FVulkanMemoryAllocator* GetAllocator() const { return Allocator; }
        VkPhysicalDevice GetPhysicalDevice() const { return PhysicalDevice; }
        VkDevice GetDevice() const { return Device; }

        const VkPhysicalDeviceFeatures& GetFeatures10() const { return Features10; }
        const VkPhysicalDeviceVulkan11Features& GetFeatures11() const { return Features11; }
        const VkPhysicalDeviceVulkan12Features& GetFeatures12() const { return Features12; }
        const VkPhysicalDeviceVulkan13Features& GetFeatures13() const { return Features13; }
        
        VkPhysicalDeviceProperties GetPhysicalDeviceProperties() const { return PhysicalDeviceProperties; }
        VkPhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties() const { return PhysicalDeviceMemoryProperties; }
    
    private:

        FMutex                                  ChildMutex;
        FVulkanMemoryAllocator*                 Allocator = nullptr;
        VkPhysicalDevice                        PhysicalDevice;
        VkDevice                                Device;

        VkPhysicalDeviceProperties              PhysicalDeviceProperties;
        VkPhysicalDeviceMemoryProperties        PhysicalDeviceMemoryProperties;
        
        VkPhysicalDeviceFeatures                Features10{};
        VkPhysicalDeviceVulkan11Features        Features11{};
        VkPhysicalDeviceVulkan12Features        Features12{};
        VkPhysicalDeviceVulkan13Features        Features13{};
    };

    class IDeviceChild
    {
    public:

        IDeviceChild(FVulkanDevice* InDevice)
            :Device(InDevice)
        {}
        
        FVulkanDevice* Device = nullptr;
    };
    
}
