#include "RtpPacketizer.h"
#include <winsock2.h>
#include <algorithm>

RtpPacketizer::RtpPacketizer(uint32_t ssrc, uint8_t payloadType) 
    : m_ssrc(ssrc), m_payloadType(payloadType) {}

RtpPacketizer::~RtpPacketizer() = default;

std::vector<RtpPacket> RtpPacketizer::Packetize(const std::vector<uint8_t>& data, uint32_t timestamp, bool isVideoNAL) {
    std::vector<RtpPacket> packets;
    if (data.empty()) return packets;

    // Se o pacote cabe inteiro num MTU
    if (data.size() <= MAX_PAYLOAD_SIZE || !isVideoNAL) {
        RtpPacket pkt;
        pkt.header.version = 2;
        pkt.header.padding = 0;
        pkt.header.extension = 0;
        pkt.header.csrcCount = 0;
        pkt.header.marker = 1; // Fim de frame/NAL
        pkt.header.payloadType = m_payloadType;
        pkt.header.sequenceNumber = htons(m_sequenceNumber++);
        pkt.header.timestamp = htonl(timestamp);
        pkt.header.ssrc = htonl(m_ssrc);
        pkt.payload = data;
        packets.push_back(std::move(pkt));
        return packets;
    }

    // Fragmentação RFC 7798 para HEVC (H.265)
    uint8_t nalHeader0 = data[0];
    uint8_t nalHeader1 = data[1];
    uint8_t nalType = (nalHeader0 >> 1) & 0x3F;

    // FU Indicator (Payload Header para HEVC FU)
    uint8_t fuIndicator0 = (nalHeader0 & 0x81) | (49 << 1); // F bit + Type 49 (FU) + LayerId (bit 0)
    uint8_t fuIndicator1 = nalHeader1;

    size_t offset = 2; // Ignorar o cabeçalho NAL original de 2 bytes
    size_t dataSize = data.size();
    bool isFirst = true;

    while (offset < dataSize) {
        size_t chunkSize = std::min(MAX_PAYLOAD_SIZE - 3, dataSize - offset); // 3 bytes de FU Header
        bool isLast = (offset + chunkSize) >= dataSize;

        RtpPacket pkt;
        pkt.header.version = 2;
        pkt.header.padding = 0;
        pkt.header.extension = 0;
        pkt.header.csrcCount = 0;
        pkt.header.marker = isLast ? 1 : 0;
        pkt.header.payloadType = m_payloadType;
        pkt.header.sequenceNumber = htons(m_sequenceNumber++);
        pkt.header.timestamp = htonl(timestamp);
        pkt.header.ssrc = htonl(m_ssrc);

        uint8_t fuHeader = nalType;
        if (isFirst) fuHeader |= 0x80; // S bit
        if (isLast) fuHeader |= 0x40;  // E bit

        pkt.payload.reserve(chunkSize + 3);
        pkt.payload.push_back(fuIndicator0);
        pkt.payload.push_back(fuIndicator1);
        pkt.payload.push_back(fuHeader);
        
        pkt.payload.insert(pkt.payload.end(), data.begin() + offset, data.begin() + offset + chunkSize);

        packets.push_back(std::move(pkt));
        
        offset += chunkSize;
        isFirst = false;
    }

    return packets;
}
