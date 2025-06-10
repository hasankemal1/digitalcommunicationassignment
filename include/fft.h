#pragma once

#include <QVector>
#include <QString>
#include <QDebug>
#include <QElapsedTimer>
#include <complex>
#include <vector>

// VkFFT baþlýk dosyal
#include <vkFFT/vkFFT.h>

// VkGPU yapýsýný ve yardýmcý fonksiyonlarý içe aktar
#include "utils_VkFFT.h"
#include <QObject>

class fft : public QObject
{
    Q_OBJECT
public:
    fft();
    ~fft();

    // FFT hesapla
    bool compute1DFFT(const QVector<float>& inputData, std::vector<float> &resultVector, std::vector<float>& timeVector, int inverse);

    // Ses spektrumu hesapla
    bool computeAudioSpectrum(const QVector<float>& audioData,
        QVector<float>& frequencies,
        QVector<float>& magnitudes,
        int sampleRate);

    // Zaman-Frekans analizi (STFT) hesapla
    bool computeSTFT(const QVector<float>& audioData,
        QVector<QVector<float>>& spectrogramData,
        int sampleRate,
        int fftSize,
        int hopSize,
        int windowType);

    // Pencere fonksiyonu uygula
    void applyWindow(QVector<float>& data, int windowType);

    // Son hata mesajýný al
    QString getLastError() const { return m_lastError; }

    // FFT boyutunu ayarla
    void setFFTSize(unsigned int size) { m_fftSize = size; }

    // FFT boyutunu al
    unsigned int getFFTSize() const { return m_fftSize; }

private:
    // Giriþ verisini hazýrla
    std::vector<float> prepareInputData(const QVector<float>& inputQVector, size_t& N_for_fft);

    // Üye deðiþkenler
    VkGPU m_vkGPU;                 // Vulkan GPU yapýsý
    bool m_isInitialized;          // Baþlatma durumu
    unsigned int m_fftSize;        // Ýleri FFT'den sonra N'i saklamak için kullanýlýr
    QString m_lastError;           // Son hata mesajý
};