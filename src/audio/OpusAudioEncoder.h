#pragma once
#include <opus.h>
#include <vector>
#include <cstdint>

class OpusAudioEncoder {
public:
    OpusAudioEncoder();
    ~OpusAudioEncoder();

    bool Initialize(int sampleRate, int channels, int bitrateBps);
    bool Encode(const std::vector<float>& pcmData, std::vector<std::vector<uint8_t>>& outPackets);

private:
    OpusEncoder* m_encoder = nullptr;
    int m_channels = 2;
    int m_sampleRate = 48000;
    
    std::vector<float> m_pcmBuffer;
    int m_frameSizeSamples;
};
