#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d11.h>
#include <vector>
#include <cstdint>
#include <string>
#include <memory>

class VideoEncoderNVENC;
class VideoEncoderAMD;

enum class GpuVendorType {
    Unknown,
    NVIDIA,
    AMD,
    Intel
};

class UniversalVideoEncoder {
public:
    UniversalVideoEncoder();
    ~UniversalVideoEncoder();

    bool Initialize(ID3D11Device* pDevice, int width, int height, int bitrateKbps = -1);
    bool SetBitrate(int bitrateKbps);
    bool EncodeFrame(ID3D11Texture2D* pNV12Texture, std::vector<std::vector<uint8_t>>& outNALUnits);

    std::string GetEncoderName() const;
    std::string GetGpuName() const { return m_gpuName; }
    GpuVendorType GetVendorType() const { return m_vendorType; }

    static GpuVendorType DetectGpuVendor(ID3D11Device* pDevice, std::string& outGpuName, uint32_t& outVendorId);

private:
    std::unique_ptr<VideoEncoderNVENC> m_nvenc;
    std::unique_ptr<VideoEncoderAMD> m_amdEncoder;

    GpuVendorType m_vendorType = GpuVendorType::Unknown;
    std::string m_gpuName = "Desconhecida";
    uint32_t m_vendorId = 0;
    bool m_usingNvenc = false;
};
