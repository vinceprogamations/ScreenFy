#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d11.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <strmif.h>
#include <codecapi.h>
#include <wrl/client.h>
#include <vector>
#include <cstdint>
#include <string>

class VideoEncoderAMD {
public:
    VideoEncoderAMD();
    ~VideoEncoderAMD();

    bool Initialize(ID3D11Device* pDevice, int width, int height, int bitrateKbps = -1);
    bool SetBitrate(int bitrateKbps);
    bool EncodeFrame(ID3D11Texture2D* pNV12Texture, std::vector<std::vector<uint8_t>>& outNALUnits);

    std::string GetEncoderName() const { return m_encoderName; }

private:
    bool CreateD3D11DeviceManager();
    bool FindHardwareEncoder();
    bool ConfigureEncoder(int bitrateKbps);
    void ExtractNALUnits(const uint8_t* pData, size_t size, std::vector<std::vector<uint8_t>>& outNALUnits);

    ID3D11Device* m_pDevice = nullptr;
    int m_width = 1920;
    int m_height = 1080;
    int m_bitrateKbps = 85000;
    std::string m_encoderName = "AMD Hardware Encoder (MFT/VCN)";

    Microsoft::WRL::ComPtr<IMFDXGIDeviceManager> m_pDxgiManager;
    UINT m_resetToken = 0;
    Microsoft::WRL::ComPtr<IMFTransform> m_pEncoder;
    Microsoft::WRL::ComPtr<ICodecAPI> m_pCodecApi;

    DWORD m_inputStreamId = 0;
    DWORD m_outputStreamId = 0;
    UINT64 m_frameIndex = 0;
    bool m_mfStarted = false;
};
