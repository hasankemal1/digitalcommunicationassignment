#pragma once
#include <vector>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <string>
#include <struct.h>
// sampleData struct'ý sadeleþtirildi ve tutarlý hale getirildi

class rawdatatowavfile
{
public:
    rawdatatowavfile(const std::vector<float>& data, const sampleData& sampleData1);

private:
    // WAV baþlýk sabitleri
    const std::vector<uint8_t> RIFF_ID = { 'R','I','F','F' };
    const std::vector<uint8_t> WAVE_ID = { 'W','A','V','E' };
    const std::vector<uint8_t> fmt_ID = { 'f','m','t',' ' };
    const std::vector<uint8_t> data_ID = { 'd','a','t','a' };

    // WAV parametreleri
    uint16_t AudioFormat;
    uint16_t NumOfChan;
    uint32_t SamplesPerSec ;
    uint16_t BitsPerSample ;
    uint32_t bytesPerSec ;
    uint16_t blockAlign ;
    uint32_t Subchunk1Size ;
    uint32_t Subchunk2Size ;
    uint32_t ChunkSize ;

    std::string filePath;
    std::string fileName;
};