

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

#include <readfile.h>



bool readAudioMetadata(const QString& filePath, AudioMetadata& metadata) {
    QString normalizedPath = QDir::toNativeSeparators(filePath);
    qDebug() << "Normalize edilmis yol:" << normalizedPath;

    // Dosya varligini kontrol et
    if (!QFile::exists(normalizedPath)) {
        qDebug() << "Dosya bulunamadi:" << normalizedPath;
        return false;
    }

    qDebug() << "Dosya metadata okuma basladi:" << normalizedPath;

    // FFmpeg ile metadata bilgilerini almaya calis
    QProcess ffmpeg;
    QStringList arguments;
    arguments << "-i" << normalizedPath << "-hide_banner";

    ffmpeg.start("ffmpeg", arguments);
    bool ffmpegSuccess = false;

    if (ffmpeg.waitForStarted(3000)) {
        ffmpeg.waitForFinished(5000);
        QByteArray output = ffmpeg.readAllStandardError();
        QString outputStr = QString::fromUtf8(output);

        qDebug() << "FFmpeg ciktisi:" << outputStr;

        // Sample rate'i cikart
        QRegularExpression sampleRateRegex(", (\\d+) Hz,");
        QRegularExpressionMatch sampleRateMatch = sampleRateRegex.match(outputStr);
        if (sampleRateMatch.hasMatch()) {
            metadata.sampleRate = sampleRateMatch.captured(1).toInt();
            qDebug() << "Ornekleme hizi (Hz):" << metadata.sampleRate;
        }

        // Kanal sayisini cikart
        QRegularExpression channelRegex(", (mono|stereo|([1-9]\\d*) channels)");
        QRegularExpressionMatch channelMatch = channelRegex.match(outputStr);
        if (channelMatch.hasMatch()) {
            QString channelStr = channelMatch.captured(1);
            if (channelStr == "mono")
                metadata.channelCount = 1;
            else if (channelStr == "stereo")
                metadata.channelCount = 2;
            else
                metadata.channelCount = channelStr.split(" ").first().toInt();

            qDebug() << "Kanal sayisi:" << metadata.channelCount;
        }

        // Bit rate'i cikart
        QRegularExpression bitrateRegex("bitrate: (\\d+) kb/s");
        QRegularExpressionMatch bitrateMatch = bitrateRegex.match(outputStr);
        if (bitrateMatch.hasMatch()) {
            metadata.bitRate = bitrateMatch.captured(1).toInt() * 1000; // kbps -> bps
            qDebug() << "Bit Rate (bps):" << metadata.bitRate;
        }

        // Sureyi cikart
        QRegularExpression durationRegex("Duration: (\\d+):(\\d+):(\\d+)\\.(\\d+)");
        QRegularExpressionMatch durationMatch = durationRegex.match(outputStr);
        if (durationMatch.hasMatch()) {
            int hours = durationMatch.captured(1).toInt();
            int minutes = durationMatch.captured(2).toInt();
            int seconds = durationMatch.captured(3).toInt();
            int milliseconds = durationMatch.captured(4).toInt();

            // Sureyi milisaniye cinsine cevir
            metadata.duration = (hours * 3600 + minutes * 60 + seconds) * 1000 + milliseconds;
            qDebug() << "Sure (ms):" << metadata.duration;
        }

        // Bit derinligini tespit et
        if (outputStr.contains("pcm_s8") || outputStr.contains("8 bits"))
            metadata.bitDepth = 8;
        else if (outputStr.contains("pcm_s16") || outputStr.contains("16 bits") || outputStr.contains("s16"))
            metadata.bitDepth = 16;
        else if (outputStr.contains("pcm_s24") || outputStr.contains("24 bits"))
            metadata.bitDepth = 24;
        else if (outputStr.contains("pcm_s32") || outputStr.contains("32 bits") || outputStr.contains("s32"))
            metadata.bitDepth = 32;
        else if (outputStr.contains("pcm_f32") || outputStr.contains("float"))
            metadata.bitDepth = 32; // float formati
        else
            metadata.bitDepth = 0;

        qDebug() << "Bit derinligi:" << metadata.bitDepth;

        ffmpegSuccess = metadata.sampleRate > 0 && metadata.channelCount > 0;
    }
    else {
        qDebug() << "FFmpeg baslatilmadi!";
    }

    // FFmpeg basarisiz olduysa veya tum bilgileri saglamadiysa QMediaPlayer'i dene
    if (!ffmpegSuccess || metadata.duration <= 0) {
        qDebug() << "QMediaPlayer ile metadata okuma deneniyor...";

        QMediaPlayer player;
        QAudioOutput audioOutput;
        player.setAudioOutput(&audioOutput);

        QEventLoop loop;
        bool loaded = false;

        QObject::connect(&player, &QMediaPlayer::mediaStatusChanged,
            [&](QMediaPlayer::MediaStatus status) {
                if (status == QMediaPlayer::LoadedMedia) {
                    qDebug() << "Medya yuklendi!";

                    // Sure bilgisini al (eger FFmpeg ile alinmadiysa)
                    if (metadata.duration <= 0) {
                        metadata.duration = player.duration();
                        qDebug() << "QMediaPlayer sure (ms):" << metadata.duration;
                    }

                    // Dosya boyutundan bit rate hesapla (eger FFmpeg ile alinmadiysa)
                    if (metadata.bitRate <= 0 && metadata.duration > 0) {
                        QFileInfo fileInfo(normalizedPath);
                        qint64 fileSize = fileInfo.size();
                        metadata.bitRate = (fileSize * 8 * 1000) / metadata.duration;
                        qDebug() << "Hesaplanan bit rate (bps):" << metadata.bitRate;
                    }

                    loaded = true;
                    loop.quit();
                }
                else if (status == QMediaPlayer::InvalidMedia) {
                    qDebug() << "Gecersiz medya dosyasi:" << player.errorString();
                    loop.quit();
                }
            });

        QObject::connect(&player, &QMediaPlayer::errorOccurred,
            [&](QMediaPlayer::Error error, const QString& errorString) {
                qDebug() << "QMediaPlayer hatasi:" << errorString << "Kod:" << error;
                loop.quit();
            });

        player.setSource(QUrl::fromLocalFile(normalizedPath));
        player.play();

        QTimer::singleShot(5000, &loop, &QEventLoop::quit);
        loop.exec();
        player.stop();
    }

    // Son care: WAV dosyasi icin dogrudan baslik okuma
    if ((metadata.sampleRate <= 0 || metadata.channelCount <= 0 || metadata.bitRate <= 0) &&
        normalizedPath.toLower().endsWith(".wav")) {

        qDebug() << "WAV basligi dogrudan okunuyor...";

        QFile file(normalizedPath);
        if (file.open(QIODevice::ReadOnly)) {
            QDataStream stream(&file);
            stream.setByteOrder(QDataStream::LittleEndian);

            // RIFF header
            char riffHeader[5] = { 0 };
            stream.readRawData(riffHeader, 4);

            if (QString(riffHeader) == "RIFF") {
                // Dosya boyutu
                quint32 fileSize;
                stream >> fileSize;

                // WAVE header
                char waveHeader[5] = { 0 };
                stream.readRawData(waveHeader, 4);

                if (QString(waveHeader) == "WAVE") {
                    // fmt chunk'i bul
                    char chunkId[5] = { 0 };
                    quint32 chunkSize;

                    bool fmtFound = false;
                    while (!stream.atEnd() && !fmtFound) {
                        stream.readRawData(chunkId, 4);
                        stream >> chunkSize;

                        if (QString(chunkId) == "fmt ") {
                            fmtFound = true;

                            quint16 formatTag;
                            quint16 numChannels;
                            quint32 sampleRate;
                            quint32 avgBytesPerSec;
                            quint16 blockAlign;
                            quint16 bitsPerSample;

                            stream >> formatTag;
                            stream >> numChannels;
                            stream >> sampleRate;
                            stream >> avgBytesPerSec;
                            stream >> blockAlign;
                            stream >> bitsPerSample;

                            // Degerleri cikart
                            metadata.sampleRate = sampleRate;
                            metadata.channelCount = numChannels;
                            metadata.bitRate = avgBytesPerSec * 8; // bytes/sec to bits/sec
                            metadata.bitDepth = bitsPerSample;  // <-- Bu satir eksikti

                            qDebug() << "WAV basligindan:";
                            qDebug() << "  Ornekleme hizi:" << metadata.sampleRate;
                            qDebug() << "  Kanal sayisi:" << metadata.channelCount;
                            qDebug() << "  Bit hizi:" << metadata.bitRate;
                            qDebug() << "  Bit derinligi:" << metadata.bitDepth;
                            break;
                        }
                        else {
                            // Diger chunk'lari atla
                            stream.skipRawData(chunkSize);
                        }
                    }
                }
            }
            file.close();
        }
    }

    // Sonuclari kontrol et
    bool success = metadata.sampleRate > 0 || metadata.duration > 0;

    qDebug() << "Metadata okuma tamamlandi, sonuc:" << (success ? "basarili" : "basarisiz");
    qDebug() << "  ornekleme hizi:" << metadata.sampleRate;
    qDebug() << "  Kanal sayisi:" << metadata.channelCount;
    qDebug() << "  Bit hizi:" << metadata.bitRate;
    qDebug() << "  Bit derinligi:" << metadata.bitDepth;
    qDebug() << "  Sure:" << metadata.duration << "ms";

    return success;
}
