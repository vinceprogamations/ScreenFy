#pragma once
#include "RtpHeader.h"
#include <vector>
#include <cstdint>

struct RtpPacket {
    RtpHeader header;
    std::vector<uint8_t> payload;
};

class RtpPacketizer {
public:
    RtpPacketizer(uint32_t ssrc, uint8_t payloadType);
    ~RtpPacketizer();

    std::vector<RtpPacket> Packetize(const std::vector<uint8_t>& data, uint32_t timestamp, bool isVideoNAL = true);
    
private:
    uint32_t m_ssrc;
    uint8_t m_payloadType;
    uint16_t m_sequenceNumber = 0;
    
    // Deixa margem para IPv4(20) + UDP(8) + RTP(12) num MTU local típico de 1400~1500
    const size_t MAX_PAYLOAD_SIZE = 1360; 
};
