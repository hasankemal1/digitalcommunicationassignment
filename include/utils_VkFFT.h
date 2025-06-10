#pragma once
#define VK_USE_PLATFORM_WIN32_KHR
#define WIN32_LEAN_AND_MEAN


#include <vkFFT/vkFFT.h>
#include <vulkan/vulkan.h>
#include <vector>

// Vulkan GPU yapısı
typedef struct {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue queue;
    int queueFamilyIndex;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkFence fence;
    
} VkGPU;

// Vulkan cihaz yönetimi
bool initializeVulkanDevice(VkGPU* vkGPU);
void cleanupVulkanResources(VkGPU* vkGPU);

// VkFFT için bellek yönetimi fonksiyonları
VkFFTResult allocateVulkanMemory(VkGPU* vkGPU, VkDeviceSize size, VkBufferUsageFlags usage,
                                VkMemoryPropertyFlags propertyFlags, VkBuffer* buffer, VkDeviceMemory* memory);

VkFFTResult mapVulkanMemory(VkGPU* vkGPU, VkDeviceMemory memory, VkDeviceSize size, void** data);

void unmapVulkanMemory(VkGPU* vkGPU, VkDeviceMemory memory);

// Vulkan komut buffer yönetimi
VkFFTResult beginVulkanCommandBuffer(VkGPU* vkGPU);
VkFFTResult endVulkanCommandBuffer(VkGPU* vkGPU);

// VkFFT yardımcı fonksiyonları
VkFFTResult createVkFFT(VkFFTApplication* app, VkFFTConfiguration config, VkGPU* vkGPU);
VkFFTResult runVkFFT(VkFFTApplication* app, VkFFTLaunchParams* launchParams, VkGPU* vkGPU, int inverse);

// Zaman ölçme fonksiyonu
double getCurrentTimestamp();