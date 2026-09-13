#include "OpusAudioEncoder.h"
#include <iostream>

OpusAudioEncoder::OpusAudioEncoder() = default;

OpusAudioEncoder::~OpusAudioEncoder() {
    if (m_encoder) {
        opus_encoder_destroy(m_encoder);
    }
}

bool OpusAudioEncoder::Initialize(int sampleRate, int channels, int bitrateBps) {
    m_sampleRate = sampleRate;
    m_channels = channels;
    
    // 10ms frame size
    m_frameSizeSamples = (sampleRate * 10) / 1000;

    int err;
    m_encoder = opus_encoder_create(sampleRate, channels, OPUS_APPLICATION_RESTRICTED_LOWDELAY, &err);
    if (err != OPUS_OK || !m_encoder) {
        std::cerr << "Falha ao criar encoder Opus: " << opus_strerror(err) << "\n";
        return false;
    }

    opus_encoder_ctl(m_encoder, OPUS_SET_BITRATE(bitrateBps));
    return true;
}

bool OpusAudioEncoder::Encode(const std::vector<float>& pcmData, std::vector<std::vector<uint8_t>>& outPackets) {
    if (!m_encoder || pcmData.empty()) return false;

    m_pcmBuffer.insert(m_pcmBuffer.end(), pcmData.begin(), pcmData.end());

    int samplesPerFrame = m_frameSizeSamples * m_channels;

    while (m_pcmBuffer.size() >= static_cast<size_t>(samplesPerFrame)) {
        std::vector<uint8_t> outBuffer(4000); 

        int bytesEncoded = opus_encode_float(m_encoder, m_pcmBuffer.data(), m_frameSizeSamples, outBuffer.data(), static_cast<opus_int32>(outBuffer.size()));
        
        if (bytesEncoded > 0) {
            outBuffer.resize(bytesEncoded);
            outPackets.push_back(std::move(outBuffer));
        }

        m_pcmBuffer.erase(m_pcmBuffer.begin(), m_pcmBuffer.begin() + samplesPerFrame);
    }

    return true;
}
