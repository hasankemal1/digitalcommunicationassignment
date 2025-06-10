#include "utils_VkFFT.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <QDebug>

#ifdef _WIN32
#include <windows.h>
#include <sys/types.h>
#include <sys/timeb.h>
#endif

// Vulkan aygıtını başlat
bool initializeVulkanDevice(VkGPU* vkGPU) {
    if (vkGPU == NULL) {
        qDebug() << "HATA: Geçersiz VkGPU işaretçisi (NULL)";
        return false;
    }

    // Yapıyı sıfırla
    memset(vkGPU, 0, sizeof(VkGPU));

    qDebug() << "Vulkan aygıtı başlatılıyor...";

    // Vulkan instance oluşturma
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "VkFFT Audio Analysis";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Audio Processing Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_4;
    qDebug() << "1";
    const char* validationLayers[] = {
        "VK_LAYER_KHRONOS_validation"
    };

    VkInstanceCreateInfo instanceInfo = {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;

    // Debug modunda validation layer'ları etkinleştir
#ifdef _DEBUG
    instanceInfo.enabledLayerCount = 1;
    instanceInfo.ppEnabledLayerNames = validationLayers;
#else
    instanceInfo.enabledLayerCount = 0;
#endif
    qDebug() << "2";

    // Gerekli uzantılar
    const char* extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    };
    qDebug() << "3";
    instanceInfo.enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]);
    instanceInfo.ppEnabledExtensionNames = extensions;

    // Instance oluştur
    VkResult result = vkCreateInstance(&instanceInfo, nullptr, &vkGPU->instance);
    if (result != VK_SUCCESS || vkGPU->instance == VK_NULL_HANDLE) {
        qDebug() << "Vulkan instance olusturulamadi!";
        vkGPU->instance = VK_NULL_HANDLE; // Güvenlik için
        return false;
    }
    qDebug() << "4";
    // Fiziksel aygıt seç
    uint32_t deviceCount = 0;
    result = vkEnumeratePhysicalDevices(vkGPU->instance, &deviceCount, nullptr);
    if (result != VK_SUCCESS || deviceCount == 0) {
        qDebug() << "Fiziksel aygıtlar alınamadı!";
        if (vkGPU->instance != VK_NULL_HANDLE)
            vkDestroyInstance(vkGPU->instance, nullptr);
        vkGPU->instance = VK_NULL_HANDLE;
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    result = vkEnumeratePhysicalDevices(vkGPU->instance, &deviceCount, devices.data());
    if (result != VK_SUCCESS) {
        qDebug() << "Fiziksel aygıtlar alınamadı!";
        vkDestroyInstance(vkGPU->instance, nullptr);
        return false;
    }

    qDebug() << "Bulunan GPU sayisi:" << deviceCount;

    // GPU özellikleri için en uygun aygıtı seç
    int bestDeviceIndex = -1; // "bulunamadı" anlamında -1

    // Tüm cihazları uygun bir tane bulmak için kontrol et
    for (uint32_t i = 0; i < deviceCount; i++) {
        if (devices[i] == VK_NULL_HANDLE) {
            qDebug() << "devices[" << i << "] VK_NULL_HANDLE!";
            continue;
        }

        // Bu cihazın gerekli uzantıları destekleyip desteklemediğini kontrol et
        uint32_t extensionCount = 0;
        result = vkEnumerateDeviceExtensionProperties(devices[i], nullptr, &extensionCount, nullptr);
        if (result != VK_SUCCESS) {
            qDebug() << "Cihaz uzantıları alınamadı!";
            continue;
        }

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(devices[i], nullptr, &extensionCount, availableExtensions.data());

        const char* deviceExtensions[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        // Tüm gerekli uzantıların desteklenip desteklenmediğini kontrol et
        bool allExtensionsSupported = true;
        for (size_t j = 0; j < (sizeof(deviceExtensions) / sizeof(deviceExtensions[0])); j++) {
            bool extFound = false;
            for (const auto& ext : availableExtensions) {
                if (strcmp(deviceExtensions[j], ext.extensionName) == 0) {
                    extFound = true;
                    break;
                }
            }
            if (!extFound) {
                allExtensionsSupported = false;
                break;
            }
        }

        // Tüm uzantılar destekleniyorsa, bu cihazı kullan
        if (allExtensionsSupported) {
            bestDeviceIndex = i;
            break;
        }
    }

    if (bestDeviceIndex < 0) {
        qDebug() << "Uygun GPU bulunamadi: Gerekli uzantilar desteklenmiyor";
        vkDestroyInstance(vkGPU->instance, nullptr);
        return false;
    }

    vkGPU->physicalDevice = devices[bestDeviceIndex];
    if (vkGPU->physicalDevice == VK_NULL_HANDLE) {
        qDebug() << "Seçilen fiziksel aygıt geçersiz (VK_NULL_HANDLE)";
        vkDestroyInstance(vkGPU->instance, nullptr);
        return false;
    }

    VkPhysicalDeviceProperties bestProperties;
    vkGetPhysicalDeviceProperties(vkGPU->physicalDevice, &bestProperties);
    qDebug() << "Secilen GPU:" << bestProperties.deviceName;

    // Kuyruk ailesi indeksi bul
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(vkGPU->physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(vkGPU->physicalDevice, &queueFamilyCount, queueFamilies.data());

    // Hesaplama kuyruğu desteği olan bir aile seç
    vkGPU->queueFamilyIndex = -1;
    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            vkGPU->queueFamilyIndex = i;
            break;
        }
    }

    if (vkGPU->queueFamilyIndex < 0) {
        qDebug() << "Hesaplama destekleyen kuyruk ailesi bulunamadi";
        vkDestroyInstance(vkGPU->instance, nullptr);
        return false;
    }

    // Mantıksal aygıt oluştur
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = vkGPU->queueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    // Kullanılacak özellikler
    VkPhysicalDeviceFeatures deviceFeatures = {};

    // Gerekli uzantılar
    const char* reqDeviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = sizeof(reqDeviceExtensions) / sizeof(reqDeviceExtensions[0]);
    deviceCreateInfo.ppEnabledExtensionNames = reqDeviceExtensions;

#ifdef _DEBUG
    deviceCreateInfo.enabledLayerCount = 1;
    deviceCreateInfo.ppEnabledLayerNames = validationLayers;
#else
    deviceCreateInfo.enabledLayerCount = 0;
#endif

    // Mantıksal aygıt oluştur
    result = vkCreateDevice(vkGPU->physicalDevice, &deviceCreateInfo, nullptr, &vkGPU->device);
    if (result != VK_SUCCESS) {
        qDebug() << "Vulkan mantiksal aygit olusturma hatasi:" << result;
        vkDestroyInstance(vkGPU->instance, nullptr);
        return false;
    }

    // Hesaplama kuyruğunu al
    vkGetDeviceQueue(vkGPU->device, vkGPU->queueFamilyIndex, 0, &vkGPU->queue);

    // Komut havuzu oluştur
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = vkGPU->queueFamilyIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    result = vkCreateCommandPool(vkGPU->device, &poolInfo, nullptr, &vkGPU->commandPool);
    if (result != VK_SUCCESS) {
        qDebug() << "Vulkan komut havuzu olusturma hatasi:" << result;
        vkDestroyDevice(vkGPU->device, nullptr);
        vkDestroyInstance(vkGPU->instance, nullptr);
        return false;
    }

    // Komut buffer oluştur
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = vkGPU->commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    result = vkAllocateCommandBuffers(vkGPU->device, &allocInfo, &vkGPU->commandBuffer);
    if (result != VK_SUCCESS) {
        qDebug() << "Vulkan komut buffer ayirma hatasi:" << result;
        vkDestroyCommandPool(vkGPU->device, vkGPU->commandPool, nullptr);
        vkDestroyDevice(vkGPU->device, nullptr);
        vkDestroyInstance(vkGPU->instance, nullptr);
        return false;
    }

    // Senkronizasyon için çit oluştur
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = 0;

    result = vkCreateFence(vkGPU->device, &fenceInfo, nullptr, &vkGPU->fence);
    if (result != VK_SUCCESS) {
        qDebug() << "Vulkan cit olusturma hatasi:" << result;
        vkFreeCommandBuffers(vkGPU->device, vkGPU->commandPool, 1, &vkGPU->commandBuffer);
        vkDestroyCommandPool(vkGPU->device, vkGPU->commandPool, nullptr);
        vkDestroyDevice(vkGPU->device, nullptr);
        vkDestroyInstance(vkGPU->instance, nullptr);
        return false;
    }

    qDebug() << "Vulkan aygıtı başarıyla başlatıldı";
    return true;
}

// Vulkan kaynaklarını temizle
void cleanupVulkanResources(VkGPU* vkGPU) {
    if (vkGPU == NULL) {
        qDebug() << "UYARI: cleanupVulkanResources - VkGPU NULL";
        return;
    }

    if (vkGPU->fence != VK_NULL_HANDLE) {
        vkDestroyFence(vkGPU->device, vkGPU->fence, nullptr);
        vkGPU->fence = VK_NULL_HANDLE;
    }

    if (vkGPU->commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(vkGPU->device, vkGPU->commandPool, nullptr);
        vkGPU->commandPool = VK_NULL_HANDLE;
    }

    if (vkGPU->device != VK_NULL_HANDLE) {
        vkDestroyDevice(vkGPU->device, nullptr);
        vkGPU->device = VK_NULL_HANDLE;
    }

    if (vkGPU->instance != VK_NULL_HANDLE) {
        vkDestroyInstance(vkGPU->instance, nullptr);
        vkGPU->instance = VK_NULL_HANDLE;
    }

    qDebug() << "Vulkan kaynakları temizlendi";
}

// VkFFT için bellek yönetimi yardımcı fonksiyonları
VkFFTResult allocateVulkanMemory(VkGPU* vkGPU, VkDeviceSize size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags propertyFlags, VkBuffer* buffer, VkDeviceMemory* memory) {
    // NULL pointer kontrolü
    if (vkGPU == NULL || buffer == NULL || memory == NULL) {
        qDebug() << "HATA: allocateVulkanMemory - Geçersiz işaretçiler";
        return VKFFT_ERROR_INVALID_CONTEXT;
    }

    // İşaretçileri varsayılan değerlerle başlat
    *buffer = VK_NULL_HANDLE;
    *memory = VK_NULL_HANDLE;

    // Boyut kontrolü
    if (size == 0) {
        qDebug() << "HATA: allocateVulkanMemory - Geçersiz boyut (0)";
        return VKFFT_ERROR_FAILED_TO_CREATE_BUFFER;
    }

    if (size > UINT32_MAX) {
        qDebug() << "HATA: Bellek boyutu çok büyük (32-bit sınırını aşıyor)";
        return VKFFT_ERROR_FAILED_TO_CREATE_BUFFER;
    }

    // Buffer oluşturma
    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = usage;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult res = vkCreateBuffer(vkGPU->device, &bufferCreateInfo, NULL, buffer);
    if (res != VK_SUCCESS) {
        qDebug() << "HATA: Buffer oluşturma hatası:" << res;
        return VKFFT_ERROR_FAILED_TO_CREATE_BUFFER;
    }

    // Bellek gereksinimlerini sorgula
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vkGPU->device, *buffer, &memRequirements);

    // Uygun bellek türünü bul
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vkGPU->physicalDevice, &memProperties);

    uint32_t memoryTypeIndex = UINT32_MAX;
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memRequirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags) {
            memoryTypeIndex = i;
            break;
        }
    }

    if (memoryTypeIndex == UINT32_MAX) {
        qDebug() << "HATA: Uygun bellek türü bulunamadı";
        vkDestroyBuffer(vkGPU->device, *buffer, NULL);
        *buffer = VK_NULL_HANDLE;
        return VKFFT_ERROR_FAILED_TO_ALLOCATE_MEMORY;
    }

    // Bellek tahsisi
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex;

    res = vkAllocateMemory(vkGPU->device, &allocInfo, NULL, memory);
    if (res != VK_SUCCESS) {
        qDebug() << "HATA: Bellek ayırma hatası:" << res;
        vkDestroyBuffer(vkGPU->device, *buffer, NULL);
        *buffer = VK_NULL_HANDLE;
        return VKFFT_ERROR_FAILED_TO_ALLOCATE_MEMORY;
    }

    // Buffer'ı belleğe bağla
    res = vkBindBufferMemory(vkGPU->device, *buffer, *memory, 0);
    if (res != VK_SUCCESS) {
        qDebug() << "HATA: Buffer-bellek bağlama hatası:" << res;
        vkFreeMemory(vkGPU->device, *memory, NULL);
        vkDestroyBuffer(vkGPU->device, *buffer, NULL);
        *buffer = VK_NULL_HANDLE;
        *memory = VK_NULL_HANDLE;
        return VKFFT_ERROR_FAILED_TO_BIND_BUFFER_MEMORY;
    }

    qDebug() << "Buffer ve bellek başarıyla oluşturuldu - Boyut:" << size << "bayt";
    return VKFFT_SUCCESS;
}

// Belleği haritalama ve veri transferi
VkFFTResult mapVulkanMemory(VkGPU* vkGPU, VkDeviceMemory memory, VkDeviceSize size, void** data) {
    // NULL pointer kontrolü
    if (vkGPU == NULL || data == NULL || memory == VK_NULL_HANDLE) {
        qDebug() << "HATA: mapVulkanMemory - Geçersiz işaretçiler";
        return VKFFT_ERROR_INVALID_CONTEXT;
    }

    VkResult res = vkMapMemory(vkGPU->device, memory, 0, size, 0, data);
    if (res != VK_SUCCESS) {
        qDebug() << "HATA: Bellek haritalama hatası:" << res;
        return VKFFT_ERROR_FAILED_TO_MAP_MEMORY;
    }
    return VKFFT_SUCCESS;
}

// Bellek haritalamasını kaldırma
void unmapVulkanMemory(VkGPU* vkGPU, VkDeviceMemory memory) {
    if (vkGPU == NULL || memory == VK_NULL_HANDLE) {
        qDebug() << "UYARI: unmapVulkanMemory - Geçersiz işaretçiler";
        return;
    }

    vkUnmapMemory(vkGPU->device, memory);
}

// Vulkan komutu başlatma
VkFFTResult beginVulkanCommandBuffer(VkGPU* vkGPU) {
    // NULL pointer kontrolü
    if (vkGPU == NULL || vkGPU->commandBuffer == VK_NULL_HANDLE) {
        qDebug() << "HATA: beginVulkanCommandBuffer - Geçersiz işaretçiler";
        return VKFFT_ERROR_INVALID_CONTEXT;
    }

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VkResult res = vkBeginCommandBuffer(vkGPU->commandBuffer, &beginInfo);
    if (res != VK_SUCCESS) {
        qDebug() << "HATA: Komut buffer başlatma hatası:" << res;
        return VKFFT_ERROR_FAILED_TO_BEGIN_COMMAND_BUFFER;
    }
    return VKFFT_SUCCESS;
}

// Vulkan komutu tamamlama ve gönderme
VkFFTResult endVulkanCommandBuffer(VkGPU* vkGPU) {
    // NULL pointer kontrolü
    if (vkGPU == NULL || vkGPU->commandBuffer == VK_NULL_HANDLE) {
        qDebug() << "HATA: endVulkanCommandBuffer - Geçersiz işaretçiler";
        return VKFFT_ERROR_INVALID_CONTEXT;
    }

    VkResult res = vkEndCommandBuffer(vkGPU->commandBuffer);
    if (res != VK_SUCCESS) {
        qDebug() << "HATA: Komut buffer sonlandırma hatası:" << res;
        return VKFFT_ERROR_FAILED_TO_END_COMMAND_BUFFER;
    }

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkGPU->commandBuffer;

    res = vkQueueSubmit(vkGPU->queue, 1, &submitInfo, vkGPU->fence);
    if (res != VK_SUCCESS) {
        qDebug() << "HATA: Kuyruk gönderme hatası:" << res;
        return VKFFT_ERROR_FAILED_TO_SUBMIT_QUEUE;
    }

    res = vkWaitForFences(vkGPU->device, 1, &vkGPU->fence, VK_TRUE, UINT64_MAX);
    if (res != VK_SUCCESS) {
        qDebug() << "HATA: Çit bekleme hatası:" << res;
        return VKFFT_ERROR_FAILED_TO_WAIT_FOR_FENCES;
    }

    res = vkResetFences(vkGPU->device, 1, &vkGPU->fence);
    if (res != VK_SUCCESS) {
        qDebug() << "HATA: Çit sıfırlama hatası:" << res;
        return VKFFT_ERROR_FAILED_TO_RESET_FENCES;
    }

    return VKFFT_SUCCESS;
}

// VkFFT uygulamasını güvenli bir şekilde oluşturan fonksiyon
VkFFTResult createVkFFT(VkFFTApplication* app, VkFFTConfiguration config, VkGPU* vkGPU) {
    if (vkGPU == NULL) {
        qDebug() << "HATA: createVkFFT - Geçersiz VkGPU işaretçisi (NULL)";
        return VKFFT_ERROR_INVALID_CONTEXT;
    }

    VkFFTConfiguration localConfig = config;
    // Yalnızca doğru atamaları tut (pointer olarak)
    localConfig.device = &vkGPU->device;
    localConfig.queue = &vkGPU->queue;
  
    localConfig.fence = &vkGPU->fence;
    localConfig.commandPool = &vkGPU->commandPool;
    localConfig.physicalDevice = &vkGPU->physicalDevice;

    qDebug() << "VkFFT yapılandırılıyor...";
    qDebug() << "  FFT Boyutu:" << localConfig.size[0];

    VkFFTResult result = initializeVkFFT(app, localConfig);
    if (result != VKFFT_SUCCESS) {
        qDebug() << "HATA: VkFFT başlatılamadı - Kod:" << result;
    } else {
        qDebug() << "VkFFT başarıyla başlatıldı";
    }

    return result;
}

// VkFFT'yi çalıştıran fonksiyon
VkFFTResult runVkFFT(VkFFTApplication* app, VkFFTLaunchParams* launchParams, VkGPU* vkGPU, int inverse) {
    if (!vkGPU || !launchParams) {
        qDebug() << "HATA: runVkFFT - Geçersiz işaretçi (NULL)";
        return VKFFT_ERROR_INVALID_CONTEXT;
    }
    if (vkGPU->commandBuffer == VK_NULL_HANDLE) {
        qDebug() << "HATA: Komut buffer geçersiz";
        return VKFFT_ERROR_INVALID_CONTEXT;
    }
  
#if(VKFFT_BACKEND==0)
    // Eğer launchParams->commandBuffer null ise, vkGPU'daki komut buffer adresini ata.
    if (launchParams->commandBuffer == nullptr) {
        launchParams->commandBuffer = &(vkGPU->commandBuffer);
    }
#endif

    VkFFTResult result = beginVulkanCommandBuffer(vkGPU);
    if (result != VKFFT_SUCCESS) {
        qDebug() << "HATA: Komut buffer başlatılamadı";
        return result;
    }

    result = VkFFTAppend(app, inverse, launchParams);
    if (result != VKFFT_SUCCESS) {
        qDebug() << "HATA: VkFFTAppend başarısız - Kod:" << result;
        VkResult endResult = vkEndCommandBuffer(vkGPU->commandBuffer);
        if (endResult != VK_SUCCESS) {
            qDebug() << "UYARI: Komut buffer sonlandırma hatası:" << endResult;
        }
        return result;
    }

    result = endVulkanCommandBuffer(vkGPU);
    if (result != VKFFT_SUCCESS) {
        qDebug() << "HATA: Komut buffer sonlandırılamadı";
        return result;
    }

    return VKFFT_SUCCESS;
}