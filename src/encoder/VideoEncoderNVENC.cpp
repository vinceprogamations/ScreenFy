#include "VideoEncoderNVENC.h"
#include "../config/SettingsManager.h"
#include <iostream>
#include <windows.h>

typedef NVENCSTATUS(NVENCAPI* PNVENCODEAPICREATEINSTANCE)(NV_ENCODE_API_FUNCTION_LIST*);

VideoEncoderNVENC::VideoEncoderNVENC() = default;

VideoEncoderNVENC::~VideoEncoderNVENC() {
    if (m_hEncoder) {
        if (m_bitstreamBuffer) {
            m_nvenc.nvEncDestroyBitstreamBuffer(m_hEncoder, m_bitstreamBuffer);
        }
        if (m_registeredResource.registeredResource) {
            m_nvenc.nvEncUnregisterResource(m_hEncoder, m_registeredResource.registeredResource);
        }
        m_nvenc.nvEncDestroyEncoder(m_hEncoder);
    }
    if (m_hNvEncModule) FreeLibrary(m_hNvEncModule);
}

bool VideoEncoderNVENC::LoadNvEncApi() {
    m_hNvEncModule = LoadLibraryA("nvEncodeAPI64.dll");
    if (!m_hNvEncModule) return false;

    auto nvEncodeAPICreateInstance = (PNVENCODEAPICREATEINSTANCE)GetProcAddress(m_hNvEncModule, "NvEncodeAPICreateInstance");
    if (!nvEncodeAPICreateInstance) return false;

    if (nvEncodeAPICreateInstance(&m_nvenc) != NV_ENC_SUCCESS) return false;
    return true;
}

bool VideoEncoderNVENC::Initialize(ID3D11Device* pDevice, int width, int height, int bitrateKbps) {
    if (!LoadNvEncApi()) {
        std::cerr << "Falha ao carregar nvEncodeAPI64.dll\n";
        return false;
    }

    if (bitrateKbps <= 0) {
        bitrateKbps = SettingsManager::Get().GetSettings().TargetBitrateKbps;
    }
    uint32_t targetBitRate = static_cast<uint32_t>(bitrateKbps) * 1000;

    m_width = width;
    m_height = height;

    NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS sessionParams = { NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER };
    sessionParams.deviceType = NV_ENC_DEVICE_TYPE_DIRECTX;
    sessionParams.device = pDevice;
    sessionParams.apiVersion = NVENCAPI_VERSION;

    if (m_nvenc.nvEncOpenEncodeSessionEx(&sessionParams, &m_hEncoder) != NV_ENC_SUCCESS) {
        std::cerr << "Falha ao abrir sessão NVENC\n";
        return false;
    }

    // Configuração H.264 Low Latency, CBR configurável, no b-frames
    NV_ENC_INITIALIZE_PARAMS initParams = { NV_ENC_INITIALIZE_PARAMS_VER };
    initParams.encodeGUID = NV_ENC_CODEC_H264_GUID;
    initParams.presetGUID = NV_ENC_PRESET_P1_GUID;
    initParams.tuningInfo = NV_ENC_TUNING_INFO_ULTRA_LOW_LATENCY;
    initParams.encodeWidth = width;
    initParams.encodeHeight = height;
    initParams.darWidth = width;
    initParams.darHeight = height;
    initParams.frameRateNum = 120;
    initParams.frameRateDen = 1;
    initParams.enablePTD = 1; 
    initParams.reportSliceOffsets = 0;
    initParams.enableSubFrameWrite = 0;

    NV_ENC_PRESET_CONFIG presetConfig = { NV_ENC_PRESET_CONFIG_VER, { NV_ENC_CONFIG_VER } };
    if (m_nvenc.nvEncGetEncodePresetConfigEx(m_hEncoder, initParams.encodeGUID, initParams.presetGUID, initParams.tuningInfo, &presetConfig) != NV_ENC_SUCCESS) return false;

    m_encodeConfig = presetConfig.presetCfg;
    m_encodeConfig.encodeCodecConfig.h264Config.idrPeriod = 60; // IDR 1x por segundo
    m_encodeConfig.encodeCodecConfig.h264Config.enableIntraRefresh = 1;
    m_encodeConfig.encodeCodecConfig.h264Config.intraRefreshPeriod = 60;
    m_encodeConfig.frameIntervalP = 1; // sem B-frames
    m_encodeConfig.rcParams.rateControlMode = NV_ENC_PARAMS_RC_CBR;
    m_encodeConfig.rcParams.averageBitRate = targetBitRate;
    m_encodeConfig.rcParams.maxBitRate = targetBitRate;

    m_initParams = initParams;
    m_initParams.encodeConfig = &m_encodeConfig;

    if (m_nvenc.nvEncInitializeEncoder(m_hEncoder, &m_initParams) != NV_ENC_SUCCESS) {
        std::cerr << "Falha ao inicializar codificador NVENC\n";
        return false;
    }

    NV_ENC_CREATE_BITSTREAM_BUFFER bitstreamParams = { NV_ENC_CREATE_BITSTREAM_BUFFER_VER };
    if (m_nvenc.nvEncCreateBitstreamBuffer(m_hEncoder, &bitstreamParams) != NV_ENC_SUCCESS) return false;
    m_bitstreamBuffer = bitstreamParams.bitstreamBuffer;

    return true;
}

bool VideoEncoderNVENC::SetBitrate(int bitrateKbps) {
    if (!m_hEncoder) return false;
    uint32_t targetBitRate = static_cast<uint32_t>(bitrateKbps) * 1000;
    m_encodeConfig.rcParams.averageBitRate = targetBitRate;
    m_encodeConfig.rcParams.maxBitRate = targetBitRate;

    NV_ENC_RECONFIGURE_PARAMS reconfigParams = { NV_ENC_RECONFIGURE_PARAMS_VER };
    reconfigParams.reInitEncodeParams = m_initParams;
    reconfigParams.reInitEncodeParams.encodeConfig = &m_encodeConfig;
    if (m_nvenc.nvEncReconfigureEncoder(m_hEncoder, &reconfigParams) == NV_ENC_SUCCESS) {
        std::cout << "[VideoEncoderNVENC] Bitrate reconfigurado para " << bitrateKbps << " Kbps." << std::endl;
        return true;
    }
    std::cerr << "[VideoEncoderNVENC] Falha ao reconfigurar bitrate via NVENC." << std::endl;
    return false;
}

bool VideoEncoderNVENC::EncodeFrame(ID3D11Texture2D* pNV12Texture, std::vector<std::vector<uint8_t>>& outNALUnits) {
    if (!m_hEncoder || !pNV12Texture) return false;

    if (!m_registeredResource.registeredResource) {
        m_registeredResource.version = NV_ENC_REGISTER_RESOURCE_VER;
        m_registeredResource.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX;
        m_registeredResource.resourceToRegister = pNV12Texture;
        m_registeredResource.width = m_width;
        m_registeredResource.height = m_height;
        m_registeredResource.pitch = m_width;
        m_registeredResource.bufferFormat = NV_ENC_BUFFER_FORMAT_NV12;
        m_registeredResource.bufferUsage = NV_ENC_INPUT_IMAGE;
        
        if (m_nvenc.nvEncRegisterResource(m_hEncoder, &m_registeredResource) != NV_ENC_SUCCESS) return false;
    }

    m_mappedResource.version = NV_ENC_MAP_INPUT_RESOURCE_VER;
    m_mappedResource.registeredResource = m_registeredResource.registeredResource;
    
    if (m_nvenc.nvEncMapInputResource(m_hEncoder, &m_mappedResource) != NV_ENC_SUCCESS) return false;

    NV_ENC_PIC_PARAMS picParams = { NV_ENC_PIC_PARAMS_VER };
    picParams.inputBuffer = m_mappedResource.mappedResource;
    picParams.bufferFmt = m_mappedResource.mappedBufferFmt;
    picParams.inputWidth = m_width;
    picParams.inputHeight = m_height;
    picParams.inputPitch = m_width;
    picParams.outputBitstream = m_bitstreamBuffer;
    picParams.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;

    NVENCSTATUS status = m_nvenc.nvEncEncodePicture(m_hEncoder, &picParams);
    
    m_nvenc.nvEncUnmapInputResource(m_hEncoder, m_mappedResource.mappedResource);

    if (status != NV_ENC_SUCCESS) return false;

    NV_ENC_LOCK_BITSTREAM lockBitstream = { NV_ENC_LOCK_BITSTREAM_VER };
    lockBitstream.outputBitstream = m_bitstreamBuffer;
    lockBitstream.doNotWait = 0;

    if (m_nvenc.nvEncLockBitstream(m_hEncoder, &lockBitstream) == NV_ENC_SUCCESS) {
        uint8_t* ptr = (uint8_t*)lockBitstream.bitstreamBufferPtr;
        uint32_t size = lockBitstream.bitstreamSizeInBytes;
        
        outNALUnits.push_back(std::vector<uint8_t>(ptr, ptr + size));
        
        m_nvenc.nvEncUnlockBitstream(m_hEncoder, lockBitstream.outputBitstream);
        return true;
    }

    return false;
}
