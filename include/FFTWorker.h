#pragma once  

#include <QObject>  
#include <QThread>  
#include <complex>  
#include <vector>  
#include <QVector>
#include "fft.h"  

class FFTWorker : public QObject
{
    Q_OBJECT

private:
    fft fftInstance;  

public:
    explicit FFTWorker(QObject* parent = nullptr);
    ~FFTWorker();

public slots:
    Q_INVOKABLE void processFFT(const QVector<float>& audioData, int inverse);

signals:
    void finished(const std::vector<float>& result, int inverse, unsigned int fftSizeUsed); 
};