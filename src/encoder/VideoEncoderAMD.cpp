#include "VideoEncoderAMD.h"
#include "../utils/Logger.h"
#include "../config/SettingsManager.h"
#include <wmcodecdsp.h>
#include <iostream>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfreadwrite.lib")

using Microsoft::WRL::ComPtr;

VideoEncoderAMD::VideoEncoderAMD() = default;

VideoEncoderAMD::~VideoEncoderAMD() {
    if (m_pEncoder) {
        m_pEncoder->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN, 0);
        m_pEncoder.Reset();
    }
    m_pCodecApi.Reset();
    m_pDxgiManager.Reset();

    if (m_mfStarted) {
        MFShutdown();
        m_mfStarted = false;
    }
}

bool VideoEncoderAMD::CreateD3D11DeviceManager() {
    HRESULT hr = MFCreateDXGIDeviceManager(&m_resetToken, &m_pDxgiManager);
    if (FAILED(hr)) {
        LOG_ERROR("ENCODER_AMD", "Falha ao criar MFCreateDXGIDeviceManager (HRESULT: " + std::to_string(hr) + ")");
        return false;
    }

    hr = m_pDxgiManager->ResetDevice(m_pDevice, m_resetToken);
    if (FAILED(hr)) {
        LOG_ERROR("ENCODER_AMD", "Falha ao associar ID3D11Device ao MF DXGI Device Manager");
        return false;
    }

    return true;
}

bool VideoEncoderAMD::FindHardwareEncoder() {
    MFT_REGISTER_TYPE_INFO inInfo = { MFMediaType_Video, MFVideoFormat_NV12 };
    MFT_REGISTER_TYPE_INFO outInfo = { MFMediaType_Video, MFVideoFormat_H264 };

    IMFActivate** ppActivate = nullptr;
    UINT32 count = 0;

    // Buscar MFTs de Hardware (AMD VCE/VCN)
    HRESULT hr = MFTEnumEx(
        MFT_CATEGORY_VIDEO_ENCODER,
        MFT_ENUM_FLAG_HARDWARE | MFT_ENUM_FLAG_SORTANDFILTER,
        &inInfo,
        &outInfo,
        &ppActivate,
        &count
    );

    if (SUCCEEDED(hr) && count > 0) {
        for (UINT32 i = 0; i < count; ++i) {
            WCHAR* friendlyName = nullptr;
            UINT32 nameLen = 0;
            if (SUCCEEDED(ppActivate[i]->GetAllocatedString(MFT_FRIENDLY_NAME_Attribute, &friendlyName, &nameLen))) {
                char nameA[256];
                WideCharToMultiByte(CP_UTF8, 0, friendlyName, -1, nameA, sizeof(nameA), nullptr, nullptr);
                m_encoderName = std::string(nameA);
                CoTaskMemFree(friendlyName);
            }

            hr = ppActivate[i]->ActivateObject(IID_PPV_ARGS(&m_pEncoder));
            if (SUCCEEDED(hr)) {
                LOG_INFO("ENCODER_AMD", "MFT Hardware ativado: " + m_encoderName);
                break;
            }
        }

        for (UINT32 i = 0; i < count; ++i) {
            ppActivate[i]->Release();
        }
        CoTaskMemFree(ppActivate);
    }

    // Se enumeração não encontrar MFT específico, carregar MFT padrão da Microsoft com aceleração D3D11
    if (!m_pEncoder) {
        // {6CA510AC-B229-476b-96B7-B158825C1C40}
        static const GUID CLSID_CMSH264EncoderMFT_Local = 
            { 0x6ca510ac, 0xb229, 0x476b, { 0x96, 0xb7, 0xb1, 0x58, 0x82, 0x5c, 0x1c, 0x40 } };
        hr = CoCreateInstance(CLSID_CMSH264EncoderMFT_Local, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pEncoder));
        if (FAILED(hr)) {
            LOG_ERROR("ENCODER_AMD", "Falha ao instanciar CLSID_CMSH264EncoderMFT");
            return false;
        }
        m_encoderName = "Windows Hardware MFT Encoder";
        LOG_INFO("ENCODER_AMD", "Fallback para Windows Hardware MFT Encoder");
    }

    return true;
}

bool VideoEncoderAMD::ConfigureEncoder(int bitrateKbps) {
    if (!m_pEncoder) return false;

    // Desbloquear MFT se for assincrono
    ComPtr<IMFAttributes> pAttributes;
    if (SUCCEEDED(m_pEncoder->GetAttributes(&pAttributes)) && pAttributes) {
        pAttributes->SetUINT32(MF_TRANSFORM_ASYNC_UNLOCK, TRUE);
    }

    // Configurar D3D11 Device Manager no MFT para Zero-Copy
    HRESULT hr = m_pEncoder->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, reinterpret_cast<ULONG_PTR>(m_pDxgiManager.Get()));
    if (FAILED(hr)) {
        LOG_WARN("ENCODER_AMD", "Aviso: MFT nao suportou mensagem MFT_MESSAGE_SET_D3D_MANAGER diretamente (HRESULT: " + std::to_string(hr) + ")");
    }

    // Obter IDs dos fluxos
    DWORD inCount = 0, outCount = 0;
    m_pEncoder->GetStreamCount(&inCount, &outCount);
    m_inputStreamId = 0;
    m_outputStreamId = 0;
    std::vector<DWORD> inStreams(inCount > 0 ? inCount : 1, 0);
    std::vector<DWORD> outStreams(outCount > 0 ? outCount : 1, 0);
    if (SUCCEEDED(m_pEncoder->GetStreamIDs(inCount, inStreams.data(), outCount, outStreams.data()))) {
        m_inputStreamId = inStreams[0];
        m_outputStreamId = outStreams[0];
    }

    // 1. Configurar Media Type de Saída (H.264)
    ComPtr<IMFMediaType> pOutputType;
    hr = MFCreateMediaType(&pOutputType);
    if (FAILED(hr)) return false;

    pOutputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pOutputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    pOutputType->SetUINT32(MF_MT_AVG_BITRATE, bitrateKbps * 1000);
    MFSetAttributeSize(pOutputType.Get(), MF_MT_FRAME_SIZE, m_width, m_height);
    MFSetAttributeRatio(pOutputType.Get(), MF_MT_FRAME_RATE, 60, 1);
    MFSetAttributeRatio(pOutputType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
    pOutputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    pOutputType->SetUINT32(MF_MT_MPEG2_PROFILE, eAVEncH264VProfile_High);

    hr = m_pEncoder->SetOutputType(m_outputStreamId, pOutputType.Get(), 0);
    if (FAILED(hr)) {
        LOG_ERROR("ENCODER_AMD", "Falha ao definir OutputType no MFT (HRESULT: " + std::to_string(hr) + ")");
        return false;
    }

    // 2. Configurar Media Type de Entrada (NV12 do D3D11)
    ComPtr<IMFMediaType> pInputType;
    bool inputTypeSet = false;

    // Tentar enumerar tipos suportados pelo MFT
    for (DWORD typeIndex = 0; ; ++typeIndex) {
        ComPtr<IMFMediaType> pAvailable;
        if (FAILED(m_pEncoder->GetInputAvailableType(m_inputStreamId, typeIndex, &pAvailable))) {
            break;
        }

        GUID subtype = GUID_NULL;
        pAvailable->GetGUID(MF_MT_SUBTYPE, &subtype);
        if (subtype == MFVideoFormat_NV12) {
            pInputType = pAvailable;
            MFSetAttributeSize(pInputType.Get(), MF_MT_FRAME_SIZE, m_width, m_height);
            MFSetAttributeRatio(pInputType.Get(), MF_MT_FRAME_RATE, 60, 1);
            MFSetAttributeRatio(pInputType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
            pInputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
            hr = m_pEncoder->SetInputType(m_inputStreamId, pInputType.Get(), 0);
            if (SUCCEEDED(hr)) {
                inputTypeSet = true;
                LOG_INFO("ENCODER_AMD", "InputType NV12 configurado com sucesso a partir dos tipos disponiveis do MFT");
                break;
            }
        }
    }

    if (!inputTypeSet) {
        // Fallback: criar novo tipo NV12 explícito
        hr = MFCreateMediaType(&pInputType);
        if (SUCCEEDED(hr)) {
            pInputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
            pInputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
            MFSetAttributeSize(pInputType.Get(), MF_MT_FRAME_SIZE, m_width, m_height);
            MFSetAttributeRatio(pInputType.Get(), MF_MT_FRAME_RATE, 60, 1);
            MFSetAttributeRatio(pInputType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
            pInputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);

            hr = m_pEncoder->SetInputType(m_inputStreamId, pInputType.Get(), 0);
            if (SUCCEEDED(hr)) {
                inputTypeSet = true;
            } else {
                LOG_ERROR("ENCODER_AMD", "Falha ao definir InputType no MFT (HRESULT: " + std::to_string(hr) + ")");
                return false;
            }
        }
    }

    // 3. Ajustar parâmetros de Ultra-Baixa Latência via ICodecAPI
    hr = m_pEncoder.As(&m_pCodecApi);
    if (SUCCEEDED(hr) && m_pCodecApi) {
        // CBR (Constant Bitrate)
        VARIANT varRateControl;
        varRateControl.vt = VT_UI4;
        varRateControl.ulVal = eAVEncCommonRateControlMode_CBR;
        m_pCodecApi->SetValue(&CODECAPI_AVEncCommonRateControlMode, &varRateControl);

        // Bitrate Alvo
        VARIANT varBitrate;
        varBitrate.vt = VT_UI4;
        varBitrate.ulVal = bitrateKbps * 1000;
        m_pCodecApi->SetValue(&CODECAPI_AVEncCommonMeanBitRate, &varBitrate);

        // Modo Low Latency
        VARIANT varLowLatency;
        varLowLatency.vt = VT_BOOL;
        varLowLatency.boolVal = VARIANT_TRUE;
        m_pCodecApi->SetValue(&CODECAPI_AVLowLatencyMode, &varLowLatency);

        // Desativar B-frames para zero atraso
        VARIANT varBFrames;
        varBFrames.vt = VT_UI4;
        varBFrames.ulVal = 0;
        m_pCodecApi->SetValue(&CODECAPI_AVEncMPVDefaultBPictureCount, &varBFrames);

        // GOP Size pequeno para recuperação rápida
        VARIANT varGop;
        varGop.vt = VT_UI4;
        varGop.ulVal = 60;
        m_pCodecApi->SetValue(&CODECAPI_AVEncMPVGOPSize, &varGop);
    }

    // Iniciar streaming no encoder
    m_pEncoder->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
    m_pEncoder->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);

    return true;
}

bool VideoEncoderAMD::Initialize(ID3D11Device* pDevice, int width, int height, int bitrateKbps) {
    if (!pDevice) return false;
    m_pDevice = pDevice;
    m_width = width;
    m_height = height;

    if (bitrateKbps <= 0) {
        bitrateKbps = SettingsManager::Get().GetSettings().TargetBitrateKbps;
    }
    m_bitrateKbps = bitrateKbps;

    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        LOG_ERROR("ENCODER_AMD", "Falha ao inicializar Media Foundation (MFStartup)");
        return false;
    }
    m_mfStarted = true;

    if (!CreateD3D11DeviceManager()) return false;
    if (!FindHardwareEncoder()) return false;
    if (!ConfigureEncoder(bitrateKbps)) return false;

    LOG_INFO("ENCODER_AMD", "Codificador AMD inicializado com sucesso: " + m_encoderName + 
                            " (" + std::to_string(width) + "x" + std::to_string(height) + 
                            " @" + std::to_string(bitrateKbps) + " Kbps)");
    return true;
}

bool VideoEncoderAMD::SetBitrate(int bitrateKbps) {
    m_bitrateKbps = bitrateKbps;
    if (m_pCodecApi) {
        VARIANT varBitrate;
        varBitrate.vt = VT_UI4;
        varBitrate.ulVal = bitrateKbps * 1000;
        if (SUCCEEDED(m_pCodecApi->SetValue(&CODECAPI_AVEncCommonMeanBitRate, &varBitrate))) {
            LOG_INFO("ENCODER_AMD", "Bitrate AMD reconfigurado para " + std::to_string(bitrateKbps) + " Kbps");
            return true;
        }
    }
    return false;
}

void VideoEncoderAMD::ExtractNALUnits(const uint8_t* pData, size_t size, std::vector<std::vector<uint8_t>>& outNALUnits) {
    if (!pData || size < 4) return;

    std::vector<size_t> startPositions;

    for (size_t i = 0; i + 3 < size; ++i) {
        if (pData[i] == 0 && pData[i + 1] == 0 && pData[i + 2] == 0 && pData[i + 3] == 1) {
            startPositions.push_back(i);
        } else if (pData[i] == 0 && pData[i + 1] == 0 && pData[i + 2] == 1) {
            startPositions.push_back(i);
        }
    }

    if (startPositions.empty()) {
        outNALUnits.push_back(std::vector<uint8_t>(pData, pData + size));
        return;
    }

    for (size_t k = 0; k < startPositions.size(); ++k) {
        size_t nStart = startPositions[k];
        size_t nEnd = (k + 1 < startPositions.size()) ? startPositions[k + 1] : size;
        if (nEnd > nStart) {
            outNALUnits.emplace_back(pData + nStart, pData + nEnd);
        }
    }
}

bool VideoEncoderAMD::EncodeFrame(ID3D11Texture2D* pNV12Texture, std::vector<std::vector<uint8_t>>& outNALUnits) {
    if (!m_pEncoder || !pNV12Texture) return false;

    // 1. Criar buffer DXGI para a textura NV12
    ComPtr<IMFMediaBuffer> pInputBuffer;
    HRESULT hr = MFCreateDXGISurfaceBuffer(__uuidof(ID3D11Texture2D), pNV12Texture, 0, FALSE, &pInputBuffer);
    if (FAILED(hr)) {
        // Fallback: criar buffer de memória convencional copiando o subresource
        D3D11_TEXTURE2D_DESC desc;
        pNV12Texture->GetDesc(&desc);
        DWORD cbSize = desc.Width * desc.Height * 3 / 2;
        hr = MFCreateMemoryBuffer(cbSize, &pInputBuffer);
        if (FAILED(hr)) return false;
    }

    // 2. Criar Sample de entrada
    ComPtr<IMFSample> pInputSample;
    hr = MFCreateSample(&pInputSample);
    if (FAILED(hr)) return false;

    pInputSample->AddBuffer(pInputBuffer.Get());
    LONGLONG hnsTime = m_frameIndex * (10000000LL / 60LL);
    pInputSample->SetSampleTime(hnsTime);
    pInputSample->SetSampleDuration(10000000LL / 60LL);
    m_frameIndex++;

    // 3. Enviar amostra para o MFT
    hr = m_pEncoder->ProcessInput(m_inputStreamId, pInputSample.Get(), 0);
    if (FAILED(hr) && hr != MF_E_NOTACCEPTING) {
        LOG_WARN("ENCODER_AMD", "ProcessInput falhou com hr=" + std::to_string(hr));
        return false;
    }

    // 4. Extrair frames codificados
    MFT_OUTPUT_DATA_BUFFER outputBuffer = {};
    outputBuffer.dwStreamID = m_outputStreamId;

    ComPtr<IMFSample> pOutputSample;
    MFCreateSample(&pOutputSample);
    ComPtr<IMFMediaBuffer> pOutMediaBuffer;
    MFCreateMemoryBuffer(m_width * m_height, &pOutMediaBuffer);
    pOutputSample->AddBuffer(pOutMediaBuffer.Get());
    outputBuffer.pSample = pOutputSample.Get();

    DWORD status = 0;
    while (true) {
        hr = m_pEncoder->ProcessOutput(0, 1, &outputBuffer, &status);
        if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
            break; // Frame processado
        }
        if (FAILED(hr)) {
            break;
        }

        if (outputBuffer.pSample) {
            ComPtr<IMFMediaBuffer> pMediaBuffer;
            if (SUCCEEDED(outputBuffer.pSample->ConvertToContiguousBuffer(&pMediaBuffer))) {
                BYTE* pData = nullptr;
                DWORD currentLength = 0;
                if (SUCCEEDED(pMediaBuffer->Lock(&pData, nullptr, &currentLength)) && pData && currentLength > 0) {
                    ExtractNALUnits(pData, currentLength, outNALUnits);
                    pMediaBuffer->Unlock();
                }
            }
        }
    }

    return !outNALUnits.empty();
}
