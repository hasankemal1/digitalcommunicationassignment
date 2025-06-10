#include "rawdatatowavfile.h"
#include <fstream>
#include <vector>
#include <cstdint>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <mainwindow.h>
rawdatatowavfile::rawdatatowavfile(const std::vector<float>& data,const sampleData& a) {
    
    uint16_t AudioFormat = 1; 
    uint16_t NumOfChan = ( a.numChannels);   
    uint16_t SampleRate = (a.sampleRate);
    uint16_t BitsPerSample =(a.bitDepth);
    uint16_t blockAlign = NumOfChan * BitsPerSample / 8;
    uint16_t bytesPerSec = SampleRate * blockAlign;
    uint16_t Subchunk1Size = 16; // PCM için sabit
    std::string RIFF_ID = "RIFF";
    std::string WAVE_ID = "WAVE";
    std::string fmt_ID = "fmt ";
    std::string data_ID = "data";

    // PCM 16-bit için veri boyutu
    Subchunk2Size = static_cast<uint32_t>(data.size() * blockAlign);
    ChunkSize = 4 + (8 + Subchunk1Size) + (8 + Subchunk2Size);
    
    std::vector<uint8_t> wavData;
    wavData.reserve(44 + Subchunk2Size);

    auto writeLE = [&](uint32_t value, size_t byteCount) {
        for (size_t i = 0; i < byteCount; ++i) {
            wavData.push_back(static_cast<uint8_t>((value >> (8 * i)) & 0xFF));
        }
    };
    auto writeLE16 = [&](uint16_t value) {
        wavData.push_back(static_cast<uint8_t>(value & 0xFF));
        wavData.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    };

    // WAV header
    wavData.insert(wavData.end(), RIFF_ID.begin(), RIFF_ID.end());
    writeLE(ChunkSize, 4);
    wavData.insert(wavData.end(), WAVE_ID.begin(), WAVE_ID.end());
    wavData.insert(wavData.end(), fmt_ID.begin(), fmt_ID.end());
    writeLE(Subchunk1Size, 4);
    writeLE16(AudioFormat);
    writeLE16(NumOfChan);
    writeLE(SampleRate, 4);
    writeLE(bytesPerSec, 4);
    writeLE16(blockAlign);
    writeLE16(BitsPerSample);
    wavData.insert(wavData.end(), data_ID.begin(), data_ID.end());
    writeLE(Subchunk2Size, 4);

    // PCM 16-bit veri yazýmý (normalize ve clamp)
    for (float sample : data) {
        float clamped = std::max(-1.0f, std::min(1.0f, sample));
        int16_t s = static_cast<int16_t>(clamped * 32767.0f);
        wavData.push_back(static_cast<uint8_t>(s & 0xFF));          // Düþük byte
        wavData.push_back(static_cast<uint8_t>((s >> 8) & 0xFF));   // Yüksek byte
    }

    std::ofstream out(a.filePath, std::ios::binary);
    if (!out) {
        std::cerr << "Dosya açýlamadý: " << a.filePath << std::endl;
        return;
    }
    out.write(reinterpret_cast<char*>(wavData.data()), wavData.size());
    out.close();

    std::cout << "16-bit WAV dosyasý oluþturuldu: " << a.filePath << " ("
        << wavData.size() << " bytes)\n";
}