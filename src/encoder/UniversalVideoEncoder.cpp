#include "UniversalVideoEncoder.h"
#include "VideoEncoderNVENC.h"
#include "VideoEncoderAMD.h"
#include "../utils/Logger.h"
#include <dxgi.h>
#include <wrl/client.h>
#include <iostream>

using Microsoft::WRL::ComPtr;

UniversalVideoEncoder::UniversalVideoEncoder() = default;
UniversalVideoEncoder::~UniversalVideoEncoder() = default;

GpuVendorType UniversalVideoEncoder::DetectGpuVendor(ID3D11Device* pDevice, std::string& outGpuName, uint32_t& outVendorId) {
    if (!pDevice) return GpuVendorType::Unknown;

    ComPtr<IDXGIDevice> pDxgiDevice;
    HRESULT hr = pDevice->QueryInterface(__uuidof(IDXGIDevice), &pDxgiDevice);
    if (FAILED(hr)) return GpuVendorType::Unknown;

    ComPtr<IDXGIAdapter> pAdapter;
    hr = pDxgiDevice->GetAdapter(&pAdapter);
    if (FAILED(hr)) return GpuVendorType::Unknown;

    DXGI_ADAPTER_DESC desc;
    hr = pAdapter->GetDesc(&desc);
    if (FAILED(hr)) return GpuVendorType::Unknown;

    outVendorId = desc.VendorId;
    char nameA[256];
    WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, nameA, sizeof(nameA), nullptr, nullptr);
    outGpuName = std::string(nameA);

    // 0x10DE = NVIDIA, 0x1002 = AMD, 0x8086 = Intel
    if (desc.VendorId == 0x1002 || desc.VendorId == 0x1022) {
        return GpuVendorType::AMD;
    } else if (desc.VendorId == 0x10DE) {
        return GpuVendorType::NVIDIA;
    } else if (desc.VendorId == 0x8086) {
        return GpuVendorType::Intel;
    }

    return GpuVendorType::Unknown;
}

bool UniversalVideoEncoder::Initialize(ID3D11Device* pDevice, int width, int height, int bitrateKbps) {
    if (!pDevice) return false;

    m_vendorType = DetectGpuVendor(pDevice, m_gpuName, m_vendorId);
    LOG_INFO("ENCODER", "GPU Detectada para Codificação: " + m_gpuName + " (Vendor ID: 0x" + std::to_string(m_vendorId) + ")");

    // 1. Se for GPU AMD (Radeon) -> Inicializar diretamente o encoder de hardware AMD
    if (m_vendorType == GpuVendorType::AMD) {
        LOG_INFO("ENCODER", "GPU AMD Radeon detectada! Ativando codificador AMD VCE/VCN...");
        m_amdEncoder = std::make_unique<VideoEncoderAMD>();
        if (m_amdEncoder->Initialize(pDevice, width, height, bitrateKbps)) {
            m_usingNvenc = false;
            LOG_INFO("ENCODER", "Codificador de Hardware AMD ativado com sucesso.");
            return true;
        }
        LOG_WARN("ENCODER", "Falha ao inicializar encoder AMD específico. Tentando fallback universal...");
    }

    // 2. Se for GPU NVIDIA -> Tentar NVENC
    if (m_vendorType == GpuVendorType::NVIDIA) {
        LOG_INFO("ENCODER", "GPU NVIDIA detectada! Tentando inicializar NVENC...");
        m_nvenc = std::make_unique<VideoEncoderNVENC>();
        if (m_nvenc->Initialize(pDevice, width, height, bitrateKbps)) {
            m_usingNvenc = true;
            LOG_INFO("ENCODER", "Codificador NVIDIA NVENC ativado com sucesso.");
            return true;
        }
        LOG_WARN("ENCODER", "Falha ao inicializar NVENC (driver ausente ou sessão ocupada). Tentando fallback...");
        m_nvenc.reset();
    }

    // 3. Fallback Universal (AMD MFT / Intel QuickSync / Microsoft Hardware MFT)
    LOG_INFO("ENCODER", "Ativando Codificador Universal de Hardware (AMD/Intel/Windows MFT)...");
    m_amdEncoder = std::make_unique<VideoEncoderAMD>();
    if (m_amdEncoder->Initialize(pDevice, width, height, bitrateKbps)) {
        m_usingNvenc = false;
        return true;
    }

    LOG_ERROR("ENCODER", "Nenhum codificador de vídeo de hardware disponível no sistema.");
    return false;
}

bool UniversalVideoEncoder::SetBitrate(int bitrateKbps) {
    if (m_usingNvenc && m_nvenc) {
        return m_nvenc->SetBitrate(bitrateKbps);
    } else if (m_amdEncoder) {
        return m_amdEncoder->SetBitrate(bitrateKbps);
    }
    return false;
}

bool UniversalVideoEncoder::EncodeFrame(ID3D11Texture2D* pNV12Texture, std::vector<std::vector<uint8_t>>& outNALUnits) {
    if (m_usingNvenc && m_nvenc) {
        return m_nvenc->EncodeFrame(pNV12Texture, outNALUnits);
    } else if (m_amdEncoder) {
        return m_amdEncoder->EncodeFrame(pNV12Texture, outNALUnits);
    }
    return false;
}

std::string UniversalVideoEncoder::GetEncoderName() const {
    if (m_usingNvenc && m_nvenc) {
        return "NVIDIA NVENC (HEVC/H.265 Ultra-Low Latency)";
    } else if (m_amdEncoder) {
        return m_amdEncoder->GetEncoderName();
    }
    return "Nenhum Codificador Ativo";
}
