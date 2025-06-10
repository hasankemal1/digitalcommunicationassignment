#include "FFTWorker.h"  // B�y�k/k���k harf e�le�mesi �nemli
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
    qDebug() << "FFTWorker::processFFT ba�lad�, inverse =" << inverse;
    std::vector<float> timeVector;    // compute1DFFT'nin timeVector ��kt�s� i�in
    std::vector<float> resultVector;  // compute1DFFT'nin resultVector ��kt�s� i�in

    if (audioData.isEmpty()) {
        qDebug() << "FFTWorker::processFFT - Gelen audioData bo�.";
        emit finished(std::vector<float>(), inverse, 0); // Bo� sonu� ve 0 FFT boyutu ile sinyal g�nder
        return;
    }

    try {
        unsigned int fftSizeForThisRun = 0; // Bu �al��madaki FFT boyutunu saklamak i�in
        if (fftInstance.compute1DFFT(audioData, resultVector, timeVector, inverse)) {
            fftSizeForThisRun = fftInstance.getFFTSize(); // Ba�ar�l� i�lem sonras� FFT boyutunu al
            qDebug() << "FFTWorker::processFFT - compute1DFFT ba�ar�l�. FFT Boyutu:" << fftSizeForThisRun;

            if (inverse == -1) { // �leri FFT ise resultVector'u g�nder
                emit finished(resultVector, inverse, fftSizeForThisRun);
            }
            else { // Ters FFT ise timeVector'u g�nder
                emit finished(timeVector, inverse, fftSizeForThisRun);
            }
        }
        else {
            qDebug() << "FFTWorker::processFFT - FFT hesaplamas� ba�ar�s�z:" << fftInstance.getLastError();
            emit finished(std::vector<float>(), inverse, 0); // Hata durumunda bo� sonu� ve 0 FFT boyutu
        }
    }
    catch (const std::exception& e) {
        qDebug() << "FFTWorker::processFFT - FFT i�lemi s�ras�nda std::exception:" << e.what();
        emit finished(std::vector<float>(), inverse, 0);
    }
    catch (...) {
        qDebug() << "FFTWorker::processFFT - FFT i�lemi s�ras�nda bilinmeyen bir hata olu�tu.";
        emit finished(std::vector<float>(), inverse, 0);
    }
    qDebug() << "FFTWorker::processFFT bitti.";
}