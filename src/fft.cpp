#include "fft.h"
#include <algorithm>
#include <cmath>
#include <QDebug>
#include <cstring> // memset, memcpy için
#include <vector>  // std::vector için
#include <utils_VkFFT.h>
#include <iostream>
#include <memory>
#include <vkFFT/vkFFT.h>
// <quadmath.h> dahil edilmeyecek

constexpr float PI = 3.14159265358979323846f;

fft::fft() : m_isInitialized(false), m_fftSize(0)
{
    m_vkGPU = {};
    m_isInitialized = initializeVulkanDevice(&m_vkGPU);
    if (!m_isInitialized) {
        qDebug() << "VkFFT: Vulkan aygiti baslatma hatasi";
    }
    else {
        qDebug() << "VkFFT: Vulkan aygiti basariyla baslatildi";
    }
}

fft::~fft()
{
    cleanupVulkanResources(&m_vkGPU);
    m_isInitialized = false;
    qDebug() << "VkFFT: Kaynaklar temizlendi ve yıkıcı çağrıldı.";
}

std::vector<float> fft::prepareInputData(const QVector<float>& inputQVector, size_t& N_for_fft)
{
    if (inputQVector.isEmpty()) {
        N_for_fft = 0;
        m_fftSize = 0;
        return {};
    }

    N_for_fft = static_cast<size_t>(inputQVector.size());
    m_fftSize = N_for_fft;

    std::vector<float> buffer(N_for_fft, 0.0f);
    for (int i = 0; i < inputQVector.size() && i < static_cast<int>(N_for_fft); ++i) {
        buffer[i] = inputQVector[i];
    }
    return buffer;
}

bool fft::compute1DFFT(const QVector<float>& inputData, std::vector<float>& resultVector, std::vector<float>& timeVector, int inverse)
{
    if (!m_isInitialized) {
        m_lastError = "VkFFT henuz baslatilmamis";
        qDebug() << m_lastError;
        return false;
    }
    if (m_vkGPU.physicalDevice == VK_NULL_HANDLE) {
        m_lastError = "Fiziksel aygıt geçersiz (VK_NULL_HANDLE)";
        qDebug() << m_lastError;
        return false;
    }

    VkFFTConfiguration localConfig = {};
    localConfig.halfPrecision = 0;
    localConfig.doublePrecision = 1; // Çift duyarlılığı etkinleştir
    localConfig.quadDoubleDoublePrecision = 0; // Dörtlü duyarlılığı devre dışı bırak
    localConfig.quadDoubleDoublePrecisionDoubleMemory = 0;
    localConfig.performR2C = 1;

    // double için eleman boyutu (gerçek veya karmaşık bileşen başına)
    size_t vkfft_component_size = sizeof(double);

    size_t N_fft;

    std::vector<unsigned char> dataForVulkanBufferBytes;
    VkDeviceSize sizeOfDataToCopyInBytes = 0;

    if (inverse == -1) { // İleri FFT
        if (inputData.isEmpty()) {
            m_lastError = "İleri FFT için boş giriş verisi";
            qDebug() << m_lastError;
            return false;
        }
        N_fft = static_cast<size_t>(inputData.size());
        m_fftSize = N_fft;
        qDebug() << "İleri FFT: inputData.size() =" << inputData.size() << ", m_fftSize ayarlandı:" << m_fftSize;


        if (N_fft == 0) {
            m_lastError = "Giriş verisi (ileri FFT) N_fft=0 sonucunu verdi.";
            qDebug() << m_lastError;
            return false;
        }
        // R2C için giriş N_fft gerçek double sayıdır
        sizeOfDataToCopyInBytes = vkfft_component_size * N_fft;
        dataForVulkanBufferBytes.resize(sizeOfDataToCopyInBytes);

        std::vector<double> temp_double_buffer(N_fft); // __float128 yerine double
        for (size_t i = 0; i < N_fft; ++i) {
            temp_double_buffer[i] = static_cast<double>(inputData[static_cast<int>(i)]); // __float128 yerine double
        }
        memcpy(dataForVulkanBufferBytes.data(), temp_double_buffer.data(), sizeOfDataToCopyInBytes);

    }
    else { // Ters FFT
        if (m_fftSize == 0) {
            m_lastError = "Ters FFT için N (m_fftSize) ayarlanmamış/sıfır. Önce ileri FFT yapılmalı.";
            qDebug() << m_lastError;
            return false;
        }
        N_fft = m_fftSize;
        qDebug() << "Ters FFT: Başlangıç N_fft (m_fftSize'dan gelen) =" << N_fft;

        // Giriş (inputData) float spektrum verisi, bunu double'a dönüştürmeliyiz
        size_t expected_input_float_elements = (N_fft / 2 + 1) * 2;
        if (inputData.isEmpty() || static_cast<size_t>(inputData.size()) != expected_input_float_elements) {
            m_lastError = QString("Ters FFT için geçersiz giriş spektrum boyutu (float[]). Gelen: %1, Beklenen: %2 (N_fft=%3).")
                .arg(inputData.size()).arg(expected_input_float_elements).arg(N_fft);
            qDebug() << m_lastError;
            return false;
        }

        // VkFFT'ye gönderilecek double karmaşık spektrum için boyut
        sizeOfDataToCopyInBytes = vkfft_component_size * expected_input_float_elements;
        dataForVulkanBufferBytes.resize(sizeOfDataToCopyInBytes);

        std::vector<double> temp_double_spectrum_buffer(expected_input_float_elements); // __float128 yerine double
        for (size_t i = 0; i < expected_input_float_elements; ++i) {
            temp_double_spectrum_buffer[i] = static_cast<double>(inputData[static_cast<int>(i)]); // __float128 yerine double
        }
        memcpy(dataForVulkanBufferBytes.data(), temp_double_spectrum_buffer.data(), sizeOfDataToCopyInBytes);
    }

    // R2C/C2R için VkFFT çalışma arabelleği (N_fft/2 + 1) karmaşık double sayı tutar.
    VkDeviceSize vulkanWorkBufferSizeInBytes = vkfft_component_size * (N_fft / 2 + 1) * 2;

    localConfig.FFTdim = 1;
    localConfig.size[0] = N_fft;
    localConfig.numberBatches = 1;
    localConfig.useLUT = 1;

    pfUINT actualVkWorkBufferSize = static_cast<pfUINT>(vulkanWorkBufferSizeInBytes);
    localConfig.bufferNum = 1;
    localConfig.bufferSize = &actualVkWorkBufferSize;

    if (inverse == 1) {
        localConfig.normalize = 1;
    }
    else {
        localConfig.normalize = 0;
    }

    VkFFTApplication localApp = {};
    VkFFTResult resFFT = createVkFFT(&localApp, localConfig, &m_vkGPU);
    if (resFFT != VKFFT_SUCCESS) {
        m_lastError = QString("VkFFT baslatma hatasi (createVkFFT): %1. N_fft=%2, performR2C=%3, normalize=%4, doublePrecision=%5") // quadDoubleDoublePrecision yerine doublePrecision
            .arg(resFFT).arg(N_fft).arg(localConfig.performR2C).arg(localConfig.normalize).arg(localConfig.doublePrecision);
        qDebug() << m_lastError;
        return false;
    }

    VkBuffer workBuffer = VK_NULL_HANDLE;
    VkDeviceMemory workMemory = VK_NULL_HANDLE;

    resFFT = allocateVulkanMemory(&m_vkGPU, vulkanWorkBufferSizeInBytes,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &workBuffer, &workMemory);

    if (resFFT != VKFFT_SUCCESS) {
        m_lastError = QString("Vulkan buffer/bellek olusturma hatasi: %1").arg(resFFT);
        deleteVkFFT(&localApp);
        qDebug() << m_lastError;
        return false;
    }

    void* mappedData = nullptr;
    resFFT = mapVulkanMemory(&m_vkGPU, workMemory, vulkanWorkBufferSizeInBytes, &mappedData);
    if (resFFT != VKFFT_SUCCESS) {
        m_lastError = QString("Vulkan bellek haritalama hatasi: %1").arg(resFFT);
        vkDestroyBuffer(m_vkGPU.device, workBuffer, nullptr);
        vkFreeMemory(m_vkGPU.device, workMemory, nullptr);
        deleteVkFFT(&localApp);
        qDebug() << m_lastError;
        return false;
    }

    memset(mappedData, 0, vulkanWorkBufferSizeInBytes);
    memcpy(mappedData, dataForVulkanBufferBytes.data(), sizeOfDataToCopyInBytes);
    unmapVulkanMemory(&m_vkGPU, workMemory);

    VkFFTLaunchParams launchParams = {};
    VkBuffer buffersForVkFFT[2] = { workBuffer, workBuffer };
    launchParams.buffer = buffersForVkFFT;

    resFFT = runVkFFT(&localApp, &launchParams, &m_vkGPU, inverse);
    if (resFFT != VKFFT_SUCCESS) {
        m_lastError = QString("VkFFT calistirma hatasi (runVkFFT): %1").arg(resFFT);
        vkDestroyBuffer(m_vkGPU.device, workBuffer, nullptr);
        vkFreeMemory(m_vkGPU.device, workMemory, nullptr);
        deleteVkFFT(&localApp);
        qDebug() << m_lastError;
        return false;
    }

    resFFT = mapVulkanMemory(&m_vkGPU, workMemory, vulkanWorkBufferSizeInBytes, &mappedData);
    if (resFFT != VKFFT_SUCCESS) {
        m_lastError = QString("Sonuc haritalama hatasi: %1").arg(resFFT);
        vkDestroyBuffer(m_vkGPU.device, workBuffer, nullptr);
        vkFreeMemory(m_vkGPU.device, workMemory, nullptr);
        deleteVkFFT(&localApp);
        qDebug() << m_lastError;
        return false;
    }

    if (mappedData != nullptr) {
        if (inverse == -1) { // İleri FFT sonucu (karmaşık spektrum)
            size_t complex_components_to_copy = (N_fft / 2 + 1) * 2;
            resultVector.resize(complex_components_to_copy);

            double* output_data = static_cast<double*>(mappedData); // __float128 yerine double
            for (size_t i = 0; i < complex_components_to_copy; ++i) {
                resultVector[i] = static_cast<float>(output_data[i]);
            }
        }
        else { // Ters FFT sonucu (zaman domen sinyali)
            timeVector.resize(N_fft);
            qDebug() << "Ters FFT: timeVector.resize(" << N_fft << ") çağrıldı. timeVector.size() =" << timeVector.size();

            double* output_data = static_cast<double*>(mappedData); // __float128 yerine double
            for (size_t i = 0; i < N_fft; ++i) {
                timeVector[i] = static_cast<float>(output_data[i]);
            }
            qDebug() << "Ters FFT: Veri kopyalandıktan sonra timeVector.size() =" << timeVector.size();
        }
    }
    else {
        m_lastError = "Haritalanmış veri (mappedData) null.";
        qDebug() << m_lastError;
        unmapVulkanMemory(&m_vkGPU, workMemory);
        vkDestroyBuffer(m_vkGPU.device, workBuffer, nullptr);
        vkFreeMemory(m_vkGPU.device, workMemory, nullptr);
        deleteVkFFT(&localApp);
        return false;
    }

    unmapVulkanMemory(&m_vkGPU, workMemory);

    vkDestroyBuffer(m_vkGPU.device, workBuffer, nullptr);
    vkFreeMemory(m_vkGPU.device, workMemory, nullptr);
    deleteVkFFT(&localApp);

    return true;
}