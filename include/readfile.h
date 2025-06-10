#ifndef READFILE_H
#define READFILE_H

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QRegularExpression>
#include <QDebug>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QEventLoop>
#include <QTimer>
#include <QFileInfo>
#include <QUrl>
#include <QDir>
#include <QProcess>

struct AudioMetadata {
    int sampleRate = 0;
    int bitRate = 0;
    int channelCount = 0;
    qint64 duration = 0;
    int bitDepth = 0;
    bool isFloat = false; 
};
bool readAudioMetadata(const QString& filePath, AudioMetadata& metadata);

#endif // READFILE_H