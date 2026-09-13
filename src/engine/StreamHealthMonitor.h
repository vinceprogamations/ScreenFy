#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d11.h>
#include <string>
#include <functional>
#include <atomic>
#include <mutex>
#include <vector>

enum class HealthStep {
    NotStarted,
    CaptureGPU,
    EncoderNVENC,
    NetworkRoute,
    PeerAck,
    Connected,
    Failed
};

struct StreamHealthStatus {
    HealthStep step = HealthStep::NotStarted;
    bool isFailed = false;
    std::string errorMessage;
    
    // Telemetria Contínua
    float fps = 0.0f;
    float pingMs = 0.0f;
    float packetLossPercent = 0.0f;
    float bitrateMbps = 0.0f;
};

class StreamHealthMonitor {
public:
    static StreamHealthMonitor& Instance();

    void Reset();
    StreamHealthStatus GetStatus() const;
    void SetStatus(HealthStep step);
    void Fail(const std::string& reason);

    // Callbacks de Hook
    bool HookCaptureFrame(ID3D11Texture2D* pTexture, ID3D11DeviceContext* pContext);
    void HookEncoderOutput(size_t totalBytes, int frameCount);
    void HookUdpPing(bool success);
    void HookPeerAck(bool success);

    // Telemetria
    void UpdateTelemetry(float fps, float pingMs, float packetLoss, float bitrateMbps);

    std::function<void(const StreamHealthStatus&)> OnStatusChanged;

private:
    StreamHealthMonitor();
    ~StreamHealthMonitor();
    StreamHealthMonitor(const StreamHealthMonitor&) = delete;
    StreamHealthMonitor& operator=(const StreamHealthMonitor&) = delete;

    mutable std::mutex m_mutex;
    StreamHealthStatus m_status;
    
    // Contadores para Heurísticas
    int m_captureBlackFrames = 0;
    int m_encoderEmptyFrames = 0;
};
