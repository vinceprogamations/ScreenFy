#pragma once
#include <cstdint>

#pragma pack(push, 1)
struct RtpHeader {
#if defined(__BIG_ENDIAN__)
    uint8_t version : 2;
    uint8_t padding : 1;
    uint8_t extension : 1;
    uint8_t csrcCount : 4;
    
    uint8_t marker : 1;
    uint8_t payloadType : 7;
#else // x86/x64 Little Endian
    uint8_t csrcCount : 4;
    uint8_t extension : 1;
    uint8_t padding : 1;
    uint8_t version : 2;
    
    uint8_t payloadType : 7;
    uint8_t marker : 1;
#endif

    uint16_t sequenceNumber;
    uint32_t timestamp;
    uint32_t ssrc;
};
#pragma pack(pop)
