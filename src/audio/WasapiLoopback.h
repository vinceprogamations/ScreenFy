#pragma once
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <wrl/client.h>
#include <vector>

class WasapiLoopback {
public:
    WasapiLoopback();
    ~WasapiLoopback();

    bool Initialize();
    bool CaptureAudio(std::vector<float>& pcmData);
    void Stop();

private:
    Microsoft::WRL::ComPtr<IMMDeviceEnumerator> m_deviceEnumerator;
    Microsoft::WRL::ComPtr<IMMDevice> m_device;
    Microsoft::WRL::ComPtr<IAudioClient> m_audioClient;
    Microsoft::WRL::ComPtr<IAudioCaptureClient> m_captureClient;

    WAVEFORMATEX* m_pwfx = nullptr;
    UINT32 m_bufferFrameCount = 0;
};
