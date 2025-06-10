#include "realtime.h"
#include <QThread>
#include <complex>
#include <vector>
#include <iostream>
#include <QDebug>
#include "fft.h"

#include <QMediaDevices>
#include <QVariant>
#include <QAudioDevice>
#include <QAudioFormat>
#include <QtMultimedia/qaudiosource.h> 
#include <QTimer>

#include <mainwindow.h>
#include "circular.h"
#define AUDIBLE_RANGE_START 20
#define AUDIBLE_RANGE_END   2000
#define NUM_SAMPLES 24000
CircularBuffer<double>  circBuffer= CircularBuffer<double>(NUM_SAMPLES);
realtime::realtime(QObject *parent)
    : QObject(parent), audioInput(nullptr)
{
    mInputBuffer.setParent(this);
    fircoff = {
0.000343961439780648      ,
0.000375345753343297      ,
0.000406979373718576      ,
0.000438852388487961      ,
0.000470457509336550      ,
0.000500751130193361      ,
0.000528138513287161      ,
0.000550485119582244      ,
0.000565155253654014      ,
0.000569078279332058      ,
0.000558841708612843      ,
0.000530809501510627      ,
0.000481262968759939      ,
0.000406560772799111      ,
0.000303313704640131      ,
0.000168569202746022      ,
-3.48147458163831e-19     ,
-0.000203909142009418     ,
-0.000443682908210548     ,
-0.000718691483899150     ,
-0.00102700743712829      ,
-0.00136528936657831      ,
-0.00172869774855126      ,
-0.00211084773223723      ,
-0.00250380278136723      ,
-0.00289811206267804      ,
-0.00328289336188824      ,
-0.00364596209485268      ,
-0.00397400570731852      ,
-0.00425280145584142      ,
-0.00446747427107608      ,
-0.00460279015951868      ,
-0.00464347943703874      ,
-0.00457458304185550      ,
-0.00438181427810170      ,
-0.00405192762239714      ,
-0.00357308570918280      ,
-0.00293521531508632      ,
-0.00213034310170873      ,
-0.00115290205714639      ,
1.16092081496753e-18      ,
0.00132835783033328       ,
0.00282909918909435       ,
0.00449597424974469       ,
0.00631950876776036       ,
0.00828702429547004       ,
0.0103827272210828        ,
0.0125878667889610        ,
0.0148809606043959        ,
0.0172380844768938        ,
0.0196332218539793        ,
0.0220386665847395        ,
0.0244254713688892        ,
0.0267639330301877        ,
0.0290241047357546        ,
0.0311763244934677        ,
0.0331917487207319        ,
0.0350428794056727        ,
0.0367040733855894        ,
0.0381520225495361        ,
0.0393661943271888        ,
0.0403292226425849        ,
0.0410272405699225        ,
0.0414501472040668        ,
0.0415918027196817        ,
0.0414501472040668        ,
0.0410272405699225        ,
0.0403292226425849        ,
0.0393661943271888        ,
0.0381520225495361        ,
0.0367040733855894        ,
0.0350428794056727        ,
0.0331917487207319        ,
0.0311763244934677        ,
0.0290241047357546        ,
0.0267639330301877        ,
0.0244254713688892        ,
0.0220386665847395        ,
0.0196332218539793        ,
0.0172380844768938        ,
0.0148809606043959        ,
0.0125878667889610        ,
0.0103827272210828        ,
0.00828702429547004       ,
0.00631950876776036       ,
0.00449597424974469       ,
0.00282909918909435       ,
0.00132835783033328       ,
1.16092081496753e-18      ,
-0.00115290205714639      ,
-0.00213034310170873      ,
-0.00293521531508632      ,
-0.00357308570918280      ,
-0.00405192762239714      ,
-0.00438181427810170      ,
-0.00457458304185550      ,
-0.00464347943703874      ,
-0.00460279015951868      ,
-0.00446747427107608      ,
-0.00425280145584142      ,
-0.00397400570731852      ,
-0.00364596209485268      ,
-0.00328289336188824      ,
-0.00289811206267804      ,
-0.00250380278136723      ,
-0.00211084773223723      ,
-0.00172869774855126      ,
-0.00136528936657831      ,
-0.00102700743712829      ,
-0.000718691483899150     ,
-0.000443682908210548     ,
-0.000203909142009418     ,
-3.48147458163831e-19     ,
0.000168569202746022      ,
0.000303313704640131      ,
0.000406560772799111      ,
0.000481262968759939      ,
0.000530809501510627      ,
0.000558841708612843      ,
0.000569078279332058      ,
0.000565155253654014      ,
0.000550485119582244      ,
0.000528138513287161      ,
0.000500751130193361      ,
0.000470457509336550      ,
0.000438852388487961      ,
0.000406979373718576      ,
0.000375345753343297      ,
0.000343961439780648
               };
    
	
}

realtime::~realtime()
{
    if (audioInput) {
        delete audioInput;
        audioInput = nullptr;
    }
    if (mInputBuffer.isOpen())
        mInputBuffer.close();
    
}

void realtime::init(QVariant device, float note) {
    QAudioDevice dev = device.value<QAudioDevice>();

    QAudioFormat format;
    format.setSampleRate(48000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);
   
    if (!dev.isFormatSupported(format)) {
        qWarning() << "Format not supported. Using preferred.";
        format = dev.preferredFormat();
    }

    if (audioInput) {
        delete audioInput;
        audioInput = nullptr;
    }

    if (mInputBuffer.isOpen()) {
        mInputBuffer.close();

    }

        mInputBuffer.open(QBuffer::ReadWrite);
        mInputBuffer.setData(QByteArray());
      
   
    audioInput = new QAudioSource(format, this);
    audioInput->setVolume(1.0f);
   
	
    notifyTimer = new QTimer(this);
    notifyTimer->setInterval(500);
    connect(notifyTimer, &QTimer::timeout, this, &realtime::processAudioIn);
    notifyTimer->start();

    audioInput->start(&mInputBuffer);
   

 

    
}

void realtime::processAudioIn() {
    mInputBuffer.seek(0);
    QByteArray audioData = mInputBuffer.readAll();
    const int16_t* rawData = reinterpret_cast<const int16_t*>(audioData.data());
    int sampleCount = audioData.size();
    circBuffer.reset();
    dc = 0.0;
    for (int i = 0; i < sampleCount / sizeof(int16_t); ++i) {

        dc += rawData[i] ;


    }
	dc /= static_cast<double>(sampleCount / sizeof(int16_t)); 
    for (int i = 0; i < sampleCount / sizeof(int16_t); ++i) {
		
        double sample = (rawData[i] - dc) / static_cast<double>(32768);
        double sampleq = 0.54 - 0.46 * cos(2 * M_PI * i / (NUM_SAMPLES - 1)); 

        circBuffer.push(sample * sampleq);

    }
  

    if (!circBuffer.ready()) 
    {
       
        return;
    }

    std::vector<double> inputSignal = circBuffer.getBuffer(); 
	QVector<float> inputSignalVec(inputSignal.begin(), inputSignal.end());
    QVector<float>firfft(NUM_SAMPLES,0.0f);
    for (int i = 0; i < 128; i++) {

        firfft[i] = fircoff[i];




    }
    std::vector<float> output;
    std::vector<float> fftOutput;
    std::vector<float> firout;


    fftinstance.compute1DFFT(firfft,firout,output,-1);
    fftinstance.compute1DFFT(inputSignalVec, fftOutput, output, -1);
    std::vector<double> magnitudeSpectrum(fftOutput.size() / 2);
    std::vector<double> firmagnitude(firout.size()/ 2);
   
    

    for (int i = 0; i < fftOutput.size() / 2; ++i) {
        double re = fftOutput[2 * i];
        double im = fftOutput[2 * i + 1];
        double magnitude = sqrt(re * re + im * im);
        magnitudeSpectrum[i] = magnitude;
        double ref = firout[2 * i];
        double imf = firout[2 * i + 1];
        double magnitudef = sqrt(ref * ref + imf * imf);
        firmagnitude[i] = magnitudef* magnitude;
    }

      
    
    emit datastream(magnitudeSpectrum, inputSignal,firmagnitude, NUM_SAMPLES);
}

void realtime::stop() {
    if (audioInput) {
        audioInput->stop();
        delete audioInput;
        audioInput = nullptr;
    }
    if (mInputBuffer.isOpen())
        mInputBuffer.close();
    if (notifyTimer) {
        notifyTimer->stop();
        notifyTimer = nullptr;
    }   
    circBuffer.reset(); 
    dc = 0.0;


}