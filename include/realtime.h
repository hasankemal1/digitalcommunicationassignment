#pragma once

#include <QObject>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSource>
#include <QBuffer>
#include <QVariant>
#include <fft.h>
#include <circular.h>

class realtime : public QObject
{
    Q_OBJECT

public:
    explicit realtime(QObject *parent = nullptr);
    ~realtime();

public slots:
    Q_INVOKABLE void init(QVariant device, float note);
    void stop();
private slots:
    void processAudioIn();

signals:
    void datastream(const std::vector<double>& result, const std::vector<double>& time,std::vector<double> firresult, unsigned int fftSizeUsed);

private:
    fft fftinstance;
    float currentNote = 0.0f;
    QBuffer mInputBuffer;
    QAudioSource *audioInput = nullptr;
    double dc = 0.0;
    QTimer* notifyTimer = nullptr;
    std::vector<double> fircoff;
};

