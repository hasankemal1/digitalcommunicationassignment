#include "FFTWorker.h"  // Büyük/küçük harf eþleþmesi önemli
#include <QThread>
#include <complex>
#include <vector>
#include <iostream>
#include <QDebug>
#include "fft.h"

FFTWorker::FFTWorker(QObject* parent)
    : QObject(parent)
{
    
}

FFTWorker::~FFTWorker()
{
}

void FFTWorker::processFFT(const QVector<float>& audioData, int inverse)
{
    qDebug() << "FFTWorker::processFFT baþladý, inverse =" << inverse;
    std::vector<float> timeVector;    // compute1DFFT'nin timeVector çýktýsý için
    std::vector<float> resultVector;  // compute1DFFT'nin resultVector çýktýsý için

    if (audioData.isEmpty()) {
        qDebug() << "FFTWorker::processFFT - Gelen audioData boþ.";
        emit finished(std::vector<float>(), inverse, 0); // Boþ sonuç ve 0 FFT boyutu ile sinyal gönder
        return;
    }

    try {
        unsigned int fftSizeForThisRun = 0; // Bu çalýþmadaki FFT boyutunu saklamak için
        if (fftInstance.compute1DFFT(audioData, resultVector, timeVector, inverse)) {
            fftSizeForThisRun = fftInstance.getFFTSize(); // Baþarýlý iþlem sonrasý FFT boyutunu al
            qDebug() << "FFTWorker::processFFT - compute1DFFT baþarýlý. FFT Boyutu:" << fftSizeForThisRun;

            if (inverse == -1) { // Ýleri FFT ise resultVector'u gönder
                emit finished(resultVector, inverse, fftSizeForThisRun);
            }
            else { // Ters FFT ise timeVector'u gönder
                emit finished(timeVector, inverse, fftSizeForThisRun);
            }
        }
        else {
            qDebug() << "FFTWorker::processFFT - FFT hesaplamasý baþarýsýz:" << fftInstance.getLastError();
            emit finished(std::vector<float>(), inverse, 0); // Hata durumunda boþ sonuç ve 0 FFT boyutu
        }
    }
    catch (const std::exception& e) {
        qDebug() << "FFTWorker::processFFT - FFT iþlemi sýrasýnda std::exception:" << e.what();
        emit finished(std::vector<float>(), inverse, 0);
    }
    catch (...) {
        qDebug() << "FFTWorker::processFFT - FFT iþlemi sýrasýnda bilinmeyen bir hata oluþtu.";
        emit finished(std::vector<float>(), inverse, 0);
    }
    qDebug() << "FFTWorker::processFFT bitti.";
}