#include "pch.h"
#include "VulkanCrashTracker.h"

namespace Lumina::RHI
{

    inline FString AftermathErrorMessage(GFSDK_Aftermath_Result result)
    {
        switch (result)
        {
        case GFSDK_Aftermath_Result_FAIL_DriverVersionNotSupported:
            return "Unsupported driver version - requires an NVIDIA R495 display driver or newer.";
        default:
            return "Aftermath Error 0x" + eastl::to_string(result);
        }
    }
    
#ifdef _WIN32
#define AFTERMATH_CHECK_ERROR(FC)                                                                       \
[&]() {                                                                                                 \
    GFSDK_Aftermath_Result _result = FC;                                                                \
    if (!GFSDK_Aftermath_SUCCEED(_result))                                                              \
    {                                                                                                   \
        MessageBoxA(0, AftermathErrorMessage(_result).c_str(), "Aftermath Error", MB_OK);               \
        exit(1);                                                                                        \
    }                                                                                                   \
}()
#else
#define AFTERMATH_CHECK_ERROR(FC)                                                                       \
[&]() {                                                                                                 \
    GFSDK_Aftermath_Result _result = FC;                                                                \
    if (!GFSDK_Aftermath_SUCCEED(_result))                                                              \
    {                                                                                                   \
        printf("%s\n", AftermathErrorMessage(_result).c_str());                                         \
        fflush(stdout);                                                                                 \
        exit(1);                                                                                        \
    }                                                                                                   \
}()
#endif
    
    FVulkanCrashTracker::~FVulkanCrashTracker()
    {
    }

    void FVulkanCrashTracker::Initialize(RHIDevice device, RHIPhysicalDevice physicalDevice)
    {
        //if (bInitialized)
        //{
        //    return;
        //}
        //
        //Device = static_cast<VkDevice>(device);
        //PhysicalDevice = static_cast<VkPhysicalDevice>(physicalDevice);
        //
        //CrashDumpDirectory = "CrashDumps/";
        //std::filesystem::create_directories(CrashDumpDirectory.c_str());
        //
        //GFSDK_Aftermath_Result result = GFSDK_Aftermath_EnableGpuCrashDumps(
        //    GFSDK_Aftermath_Version_API,
        //    GFSDK_Aftermath_GpuCrashDumpWatchedApiFlags_Vulkan,
        //    GFSDK_Aftermath_GpuCrashDumpFeatureFlags_DeferDebugInfoCallbacks,
        //    GpuCrashDumpCallback,
        //    ShaderDebugInfoCallback,
        //    CrashDumpDescriptionCallback,
        //    ResolveMarkerCallback,
        //    this
        //);
        //
        //if (result != GFSDK_Aftermath_Result_Success)
        //{
        //    LOG_ERROR("Failed to initialize Nvidia Aftermath: {}", static_cast<int>(result));
        //    return;
        //}
        //
        //bInitialized = true;
        //LOG_INFO("Nvidia Aftermath crash tracker initialized (Vulkan)");
    }

    void FVulkanCrashTracker::Shutdown()
    {
        //if (!bInitialized)
        //{
        //    return;
        //}
        //
        //GFSDK_Aftermath_DisableGpuCrashDumps();
        //
        //RegisteredShaders.clear();
        //Markers.clear();
        //Device = VK_NULL_HANDLE;
        //PhysicalDevice = VK_NULL_HANDLE;
        //bInitialized = false;
        //
        //LOG_INFO("Nvidia Aftermath crash tracker shut down");
    }

    void FVulkanCrashTracker::OnDeviceLost()
    {
//        GFSDK_Aftermath_CrashDump_Status Status = GFSDK_Aftermath_CrashDump_Status_Unknown;
//        AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GetCrashDumpStatus(&Status));
//
//        auto TerminationTimeout = std::chrono::seconds(10);
//        auto tStart = std::chrono::steady_clock::now();
//        auto tElapsed = std::chrono::milliseconds::zero();
//
//        // Loop while Aftermath crash dump data collection has not finished or
//        // the application is still processing the crash dump data.
//        while (Status != GFSDK_Aftermath_CrashDump_Status_CollectingDataFailed && Status != GFSDK_Aftermath_CrashDump_Status_Finished && tElapsed < TerminationTimeout)
//        {
//            // Sleep a couple of milliseconds and poll the status again.
//            std::this_thread::sleep_for(std::chrono::milliseconds(50));
//            AFTERMATH_CHECK_ERROR(GFSDK_Aftermath_GetCrashDumpStatus(&Status));
//
//            tElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - tStart);
//        }
//
//        if (Status == GFSDK_Aftermath_CrashDump_Status_Finished)
//        {
//            LOG_INFO("Aftermath finished processing crash dump");
//        }
//        else
//        {
//            LOG_ERROR("Unexpected crash dump status");
//        }
//
        //std::terminate();
    }

    void FVulkanCrashTracker::EnableDeviceFeatures(vkb::DeviceBuilder& Builder)
    {
        //static VkDeviceDiagnosticsConfigCreateInfoNV DiagnosticsConfig = {};
        //DiagnosticsConfig.sType = VK_STRUCTURE_TYPE_DEVICE_DIAGNOSTICS_CONFIG_CREATE_INFO_NV;
        //DiagnosticsConfig.flags = VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_SHADER_DEBUG_INFO_BIT_NV
        //                        | VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_RESOURCE_TRACKING_BIT_NV
        //                        | VK_DEVICE_DIAGNOSTICS_CONFIG_ENABLE_AUTOMATIC_CHECKPOINTS_BIT_NV;
        //
        //
        //Builder.add_pNext(&DiagnosticsConfig);
    }

    void FVulkanCrashTracker::RegisterShader(const TVector<uint32>& SPRIV, const FString& Name)
    {
        
    }

    void FVulkanCrashTracker::SetMarker(RHICommandBuffer cmdBuffer, const char* markerName)
    {
    }

    void FVulkanCrashTracker::BeginMarker(RHICommandBuffer cmdBuffer, const char* markerName)
    {
    }

    void FVulkanCrashTracker::EndMarker(RHICommandBuffer cmdBuffer)
    {
    }

    void FVulkanCrashTracker::PollCrashDumps()
    {
    }

    void FVulkanCrashTracker::GpuCrashDumpCallback(const void* GpuCrashDump, uint32 gpuCrashDumpSize, void* UserData)
    {
        LOG_ERROR("Crash Detected!");
    }

    void FVulkanCrashTracker::ShaderDebugInfoCallback(const void* ShaderDebugInfo, uint32 shaderDebugInfoSize, void* UserData)
    {
    }

    void FVulkanCrashTracker::CrashDumpDescriptionCallback(PFN_GFSDK_Aftermath_AddGpuCrashDumpDescription addDescription, void* UserData)
    {
    }

    void FVulkanCrashTracker::ResolveMarkerCallback(const void* MarkerData, uint32 MarkerDataSize, void* UserData, PFN_GFSDK_Aftermath_ResolveMarker ResolveMarker)
    {
    }
}
