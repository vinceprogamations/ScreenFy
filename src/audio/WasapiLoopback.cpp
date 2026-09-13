#include "WasapiLoopback.h"
#include <iostream>
#include <avrt.h>

#pragma comment(lib, "avrt.lib")

const CLSID CLSID_MMDeviceEnumerator_ = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator_ = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient_ = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient_ = __uuidof(IAudioCaptureClient);

#define REFTIMES_PER_SEC  10000000

WasapiLoopback::WasapiLoopback() = default;

WasapiLoopback::~WasapiLoopback() {
    Stop();
    if (m_pwfx) CoTaskMemFree(m_pwfx);
}

bool WasapiLoopback::Initialize() {
    HRESULT hr = CoInitialize(nullptr);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) return false;

    hr = CoCreateInstance(CLSID_MMDeviceEnumerator_, nullptr, CLSCTX_ALL, IID_IMMDeviceEnumerator_, (void**)&m_deviceEnumerator);
    if (FAILED(hr)) return false;

    hr = m_deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_device);
    if (FAILED(hr)) return false;

    hr = m_device->Activate(IID_IAudioClient_, CLSCTX_ALL, nullptr, (void**)&m_audioClient);
    if (FAILED(hr)) return false;

    hr = m_audioClient->GetMixFormat(&m_pwfx);
    if (FAILED(hr)) return false;

    REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
    hr = m_audioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_LOOPBACK, // Lote 2: Loopback
        hnsRequestedDuration,
        0,
        m_pwfx,
        nullptr
    );
    if (FAILED(hr)) return false;

    hr = m_audioClient->GetBufferSize(&m_bufferFrameCount);
    if (FAILED(hr)) return false;

    hr = m_audioClient->GetService(IID_IAudioCaptureClient_, (void**)&m_captureClient);
    if (FAILED(hr)) return false;

    hr = m_audioClient->Start();
    if (FAILED(hr)) return false;

    // Registo na classe MMCSS Pro Audio
    DWORD taskIndex = 0;
    HANDLE hTask = AvSetMmThreadCharacteristicsA("Pro Audio", &taskIndex);
    if (hTask == nullptr) {
        std::cerr << "Warning: Falha ao setar MMCSS Pro Audio no WASAPI.\n";
    }

    return true;
}

bool WasapiLoopback::CaptureAudio(std::vector<float>& pcmData) {
    if (!m_captureClient) return false;

    UINT32 packetLength = 0;
    HRESULT hr = m_captureClient->GetNextPacketSize(&packetLength);
    if (FAILED(hr)) return false;

    while (packetLength != 0) {
        BYTE* pData;
        UINT32 numFramesAvailable;
        DWORD flags;

        hr = m_captureClient->GetBuffer(&pData, &numFramesAvailable, &flags, nullptr, nullptr);
        if (FAILED(hr)) return false;

        if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
            pcmData.insert(pcmData.end(), numFramesAvailable * m_pwfx->nChannels, 0.0f);
        } else {
            float* fData = reinterpret_cast<float*>(pData);
            pcmData.insert(pcmData.end(), fData, fData + (numFramesAvailable * m_pwfx->nChannels));
        }

        hr = m_captureClient->ReleaseBuffer(numFramesAvailable);
        if (FAILED(hr)) return false;

        hr = m_captureClient->GetNextPacketSize(&packetLength);
        if (FAILED(hr)) return false;
    }
    return true;
}

void WasapiLoopback::Stop() {
    if (m_audioClient) {
        m_audioClient->Stop();
    }
}
