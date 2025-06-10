#pragma once
#include <string> 
#include <cstdint>
struct sampleData {
	
	uint16_t	sampleRate;
	uint16_t	numChannels;
	uint16_t	bitDepth;
	uint32_t	bitRate;
	float		duration;
	std::string	fileName;
	std::string	filePath;


};