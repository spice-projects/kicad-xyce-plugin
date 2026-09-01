// #include <vector>

#include <gpu/ganesh/GrDirectContext.h>
#include <gpu/ganesh/vk/GrVkDirectContext.h>
#include <gpu/vk/VulkanBackendContext.h>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.h>

#include "charts_renderer.h"

struct VulkanCleanup
{

    VkInstance instance = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    ~VulkanCleanup() {
        // destroy device
        if (device != VK_NULL_HANDLE)
            vkDestroyDevice(device, nullptr);
        // destroy instance
        if (instance != VK_NULL_HANDLE)
            vkDestroyInstance(instance, nullptr);
    }
};

sk_sp<GrDirectContext> ChartsRenderer::create_gpu_context() {
    // // cleanup if early exit
    // VulkanCleanup tracker;
    // // create the Vulkan Instance
    // VkApplicationInfo appInfo = {};
    // appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    // appInfo.pApplicationName = "ChartsRenderer";
    // appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    // appInfo.pEngineName = "SkiaEngine";
    // appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    // appInfo.apiVersion = VK_API_VERSION_1_1;
    // // 
    // VkInstanceCreateInfo instanceInfo = {};
    // instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    // instanceInfo.pApplicationInfo = &appInfo;
    // // create vulkan instance
    // if (vkCreateInstance(&instanceInfo, nullptr, &tracker.instance) != VK_SUCCESS)
    //     return nullptr;
    // // select a suitable Physical Device (GPU)
    // uint32_t deviceCount = 0;
    // vkEnumeratePhysicalDevices(tracker.instance, &deviceCount, nullptr);
    // if (deviceCount == 0) {
    //     // log information
    //     spdlog::debug("No GPU physical devices found, using software rasterizer");
    //     // exit
    //     return nullptr;
    // }
    // // allocate vector with device count    
    // std::vector<VkPhysicalDevice> physicalDevices(deviceCount);
    // // fill vector with device information
    // vkEnumeratePhysicalDevices(tracker.instance, &deviceCount, physicalDevices.data());
    // // choose the first available physical device
    // VkPhysicalDevice physicalDevice = physicalDevices[0];
    // // find a Graphics Queue Family Index
    // uint32_t queueFamilyCount = 0;
    // vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    // if (queueFamilyCount == 0) {
    //     // log information
    //     spdlog::debug("Cannot find Queue Family count for device: {}", physicalDevice);
    //     // exit
    //     return nullptr;
    // }

    // std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    // vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    // int graphicsQueueIndex = -1;
    // for (uint32_t i = 0; i < queueFamilyCount; i++) {
    //     if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
    //         graphicsQueueIndex = i;
    //         break;
    //     }
    // }
    // if (graphicsQueueIndex == -1) {
    //     return nullptr;
    // }

    // // 4. Create the Logical Device and Graphics Queue
    // float queuePriority = 1.0f;
    // VkDeviceQueueCreateInfo queueCreateInfo = {};
    // queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    // queueCreateInfo.queueFamilyIndex = static_cast<uint32_t>(graphicsQueueIndex);
    // queueCreateInfo.queueCount = 1;
    // queueCreateInfo.pQueuePriorities = &queuePriority;

    // // Retrieve default device features supported by the selected hardware
    // VkPhysicalDeviceFeatures deviceFeatures = {};
    // vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

    // VkDeviceCreateInfo deviceCreateInfo = {};
    // deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    // deviceCreateInfo.queueCreateInfoCount = 1;
    // deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    // deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    // if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &tracker.device) != VK_SUCCESS) {
    //     return nullptr;
    // }

    // VkQueue queue = VK_NULL_HANDLE;
    // vkGetDeviceQueue(tracker.device, static_cast<uint32_t>(graphicsQueueIndex), 0, &queue);

    // // 5. Define Skia Dynamic Loader Proc Interface
    // // Critical: Skia uses this to link device driver mappings without statically linking libraries
    // skgpu::VulkanGetProc vkGetProc = [](const char* procName, VkInstance instance, VkDevice device) {
    //     if (device != VK_NULL_HANDLE) {
    //         auto proc = vkGetDeviceProcAddr(device, procName);
    //         if (proc)
    //             return proc;
    //     }
    //     return vkGetInstanceProcAddr(instance, procName);
    // };

    // // 6. Assemble the Skia Backend Configuration Context
    // skgpu::VulkanBackendContext backend;
    // backend.fInstance = tracker.instance;
    // backend.fPhysicalDevice = physicalDevice;
    // backend.fDevice = tracker.device;
    // backend.fQueue = queue;
    // backend.fGraphicsQueueIndex = static_cast<uint32_t>(graphicsQueueIndex);
    // backend.fMaxAPIVersion = VK_API_VERSION_1_1;
    // backend.fGetProc = vkGetProc;

    // // 7. Initialize Direct Context
    // GrContextOptions options;
    // sk_sp<GrDirectContext> directContext = GrDirectContexts::MakeVulkan(backend, options);

    // if (directContext) {
    //     // Ownership Transfer: Release tracker tracking variables so raw handles
    //     // persist for Skia runtime use.
    //     tracker.instance = VK_NULL_HANDLE;
    //     tracker.device = VK_NULL_HANDLE;
    // }

    // return directContext;
    return nullptr;
}
