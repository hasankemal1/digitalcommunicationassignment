#pragma once

#include <QVector>
#include <QString>
#include <QDebug>
#include <QElapsedTimer>
#include <complex>
#include <vector>

// VkFFT ba�l�k dosyal
#include <vkFFT/vkFFT.h>

// VkGPU yap�s�n� ve yard�mc� fonksiyonlar� i�e aktar
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

    // Son hata mesaj�n� al
    QString getLastError() const { return m_lastError; }

    // FFT boyutunu ayarla
    void setFFTSize(unsigned int size) { m_fftSize = size; }

    // FFT boyutunu al
    unsigned int getFFTSize() const { return m_fftSize; }

private:
    // Giri� verisini haz�rla
    std::vector<float> prepareInputData(const QVector<float>& inputQVector, size_t& N_for_fft);

    // �ye de�i�kenler
    VkGPU m_vkGPU;                 // Vulkan GPU yap�s�
    bool m_isInitialized;          // Ba�latma durumu
    unsigned int m_fftSize;        // �leri FFT'den sonra N'i saklamak i�in kullan�l�r
    QString m_lastError;           // Son hata mesaj�
};