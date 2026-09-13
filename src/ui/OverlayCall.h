#pragma once
#include <string>
#include <functional>
#include <d3d11.h>

class OverlayCall {
public:
    OverlayCall();
    ~OverlayCall() = default;

    void SetInCall(bool inCall, bool isHost, const std::string& peerName, const std::string& peerIp);
    bool IsInCall() const { return m_isInCall; }

    void Render(float screenWidth, float screenHeight, ID3D11ShaderResourceView* pVideoSRV = nullptr);

    std::function<void()> OnEndCall;
    std::function<void(bool isMuted)> OnToggleMute;

    void UpdateTelemetry(float fps, float latencyMs, float bitrateMbps, float packetLoss);

private:
    bool m_isInCall = false;
    bool m_isHost = false;
    std::string m_peerName;
    std::string m_peerIp;
    bool m_isMuted = false;

    float m_fps = 120.0f;
    float m_latencyMs = 2.1f;
    float m_bitrateMbps = 85.0f;
    float m_packetLoss = 0.0f;

    float m_hoverTimer = 3.0f;
};
