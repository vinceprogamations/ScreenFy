#pragma once
#include <d3d11.h>
#include <vector>
#include <cstdint>
#include <nvEncodeAPI.h>

class VideoEncoderNVENC {
public:
    VideoEncoderNVENC();
    ~VideoEncoderNVENC();

    bool Initialize(ID3D11Device* pDevice, int width, int height, int bitrateKbps = -1);
    bool SetBitrate(int bitrateKbps);
    bool EncodeFrame(ID3D11Texture2D* pNV12Texture, std::vector<std::vector<uint8_t>>& outNALUnits);

private:
    bool LoadNvEncApi();
    
    HMODULE m_hNvEncModule = nullptr;
    NV_ENCODE_API_FUNCTION_LIST m_nvenc = { NV_ENCODE_API_FUNCTION_LIST_VER };
    void* m_hEncoder = nullptr;

    int m_width;
    int m_height;
    
    NV_ENC_INITIALIZE_PARAMS m_initParams = {};
    NV_ENC_CONFIG m_encodeConfig = {};

    NV_ENC_REGISTER_RESOURCE m_registeredResource = {};
    NV_ENC_MAP_INPUT_RESOURCE m_mappedResource = {};
    NV_ENC_OUTPUT_PTR m_bitstreamBuffer = nullptr;
};
