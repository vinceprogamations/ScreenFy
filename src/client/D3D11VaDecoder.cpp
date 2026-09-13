#include "D3D11VaDecoder.h"
#include <mfidl.h>
#include <mferror.h>
#include <wmcodecdsp.h>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "mf.lib")

D3D11VaDecoder::D3D11VaDecoder() {
    MFStartup(MF_VERSION);
}

D3D11VaDecoder::~D3D11VaDecoder() {
    MFShutdown();
}

bool D3D11VaDecoder::Initialize(ID3D11Device* pDevice) {
    HRESULT hr = CoCreateInstance(CLSID_MSH265DecoderMFT, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_decoder));
    if (FAILED(hr)) return false;

    UINT resetToken = 0;
    hr = MFCreateDXGIDeviceManager(&resetToken, &m_devManager);
    if (FAILED(hr)) return false;
    
    m_devManager->ResetDevice(pDevice, resetToken);
    
    Microsoft::WRL::ComPtr<IMFAttributes> pAttributes;
    hr = m_decoder->GetAttributes(&pAttributes);
    if (SUCCEEDED(hr)) {
        pAttributes->SetUINT32(MF_SA_D3D11_AWARE, TRUE);
    }
    
    m_decoder->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, reinterpret_cast<ULONG_PTR>(m_devManager.Get()));

    Microsoft::WRL::ComPtr<IMFMediaType> pInputType;
    MFCreateMediaType(&pInputType);
    pInputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pInputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_HEVC);
    m_decoder->SetInputType(0, pInputType.Get(), 0);

    Microsoft::WRL::ComPtr<IMFMediaType> pOutputType;
    MFCreateMediaType(&pOutputType);
    pOutputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    pOutputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
    m_decoder->SetOutputType(0, pOutputType.Get(), 0);
    
    m_decoder->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
    m_decoder->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);

    return true;
}

bool D3D11VaDecoder::DecodeNAL(const std::vector<uint8_t>& nalUnit) {
    if (!m_decoder || nalUnit.empty()) return false;
    
    Microsoft::WRL::ComPtr<IMFSample> pSample;
    MFCreateSample(&pSample);
    
    Microsoft::WRL::ComPtr<IMFMediaBuffer> pBuffer;
    MFCreateMemoryBuffer(static_cast<DWORD>(nalUnit.size()), &pBuffer);
    
    BYTE* pData = nullptr;
    pBuffer->Lock(&pData, nullptr, nullptr);
    memcpy(pData, nalUnit.data(), nalUnit.size());
    pBuffer->Unlock();
    pBuffer->SetCurrentLength(static_cast<DWORD>(nalUnit.size()));
    
    pSample->AddBuffer(pBuffer.Get());
    
    HRESULT hr = m_decoder->ProcessInput(0, pSample.Get(), 0);
    return SUCCEEDED(hr);
}

bool D3D11VaDecoder::GetDecodedTexture(ID3D11Texture2D** ppTexture) {
    if (!m_decoder) return false;

    MFT_OUTPUT_DATA_BUFFER outputDataBuffer = {};
    DWORD status = 0;
    
    HRESULT hr = m_decoder->ProcessOutput(0, 1, &outputDataBuffer, &status);
    if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) return false;
    
    if (SUCCEEDED(hr) && outputDataBuffer.pSample) {
        Microsoft::WRL::ComPtr<IMFMediaBuffer> pBuffer;
        outputDataBuffer.pSample->GetBufferByIndex(0, &pBuffer);
        
        Microsoft::WRL::ComPtr<IMFDXGIBuffer> pDXGIBuffer;
        if (SUCCEEDED(pBuffer.As(&pDXGIBuffer))) {
            pDXGIBuffer->GetResource(IID_PPV_ARGS(ppTexture));
        }
        
        outputDataBuffer.pSample->Release();
        if (outputDataBuffer.pEvents) outputDataBuffer.pEvents->Release();
        return (*ppTexture) != nullptr;
    }
    return false;
}
