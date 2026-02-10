#pragma once

#include <mutex>
#include <string>
#include <volk/volk.h>
#include "Containers/Name.h"
#include "Renderer/ErrorHandling/CrashTracker.h"
#if defined(WITH_AFTERMATH)
#include <NvidiaAftermath/GFSDK_Aftermath_GpuCrashDump.h>
#endif
#include "VkBootstrap.h"

namespace Lumina::RHI
{
    class FVulkanCrashTracker : public ICrashTracker
    {
    public:
        
        FVulkanCrashTracker() = default;
        ~FVulkanCrashTracker() override;
        
        void Initialize(RHIDevice device, RHIPhysicalDevice physicalDevice) override;
        void Shutdown() override;

        void OnDeviceLost() override;
        
        // Call after device creation to enable Aftermath features
        void EnableDeviceFeatures(vkb::DeviceBuilder& Builder);
        
        void RegisterShader(const TVector<uint32>& SPRIV, const FString& Name) override;
        void SetMarker(RHICommandBuffer cmdBuffer, const char* markerName) override;
        void BeginMarker(RHICommandBuffer cmdBuffer, const char* markerName) override;
        void EndMarker(RHICommandBuffer cmdBuffer) override;
        void PollCrashDumps() override;
        
    private:
        struct FShaderInfo
        {
            TVector<uint32> SPIRV;
            FName Name;
        };
        
        struct FMarkerData
        {
            FString Name;
            uint64 MarkerValue;
        };
        
        VkDevice Device = VK_NULL_HANDLE;
        VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
        
        FMutex ShaderMutex;
        TVector<FShaderInfo> RegisteredShaders;
        
        FMutex MarkerMutex;
        TVector<FMarkerData> Markers;
        uint64 NextMarkerValue = 1;
        
        FString CrashDumpDirectory;
        bool bInitialized = false;
        
        // Aftermath callbacks
        static void GpuCrashDumpCallback(const void* GpuCrashDump, uint32 gpuCrashDumpSize, void* UserData);
        static void ShaderDebugInfoCallback(const void* ShaderDebugInfo, uint32 shaderDebugInfoSize, void* UserData);
#if defined(WITH_AFTERMATH)
        static void CrashDumpDescriptionCallback(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription, void* UserData);
        static void ResolveMarkerCallback(const void* MarkerData, uint32 MarkerDataSize, void* UserData, PFN_GFSDK_Aftermath_ResolveMarker ResolveMarker);
#endif

        void WriteCrashDump(const void* data, uint32_t size);
        void DecodeAndLogCrashDump(const std::string& dumpPath);
        //std::string GetShaderNameFromHash(const GFSDK_Aftermath_ShaderBinaryHash& hash);
        FString GetMarkerName(uint64_t markerValue);
        void SaveShaderDebugInfo(const void* data, uint32_t size);
    };
    
}
