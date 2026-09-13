#pragma once
#include <string>
#include <functional>
#include <d3d11.h>

enum class ViewState {
    Inline,
    Fullscreen,
    PiP
};

class OverlayCall {
public:
    OverlayCall();
    ~OverlayCall() = default;

    void SetInCall(bool inCall, bool isHost, const std::string& peerName, const std::string& peerIp);
    bool IsInCall() const { return m_isInCall; }

    void Render(float posX, float posY, float width, float height, ID3D11ShaderResourceView* pVideoSRV = nullptr);

    std::function<void()> OnEndCall;
    std::function<void(bool isMuted)> OnToggleMute;

    void UpdateTelemetry(float fps, float latencyMs, float bitrateMbps, float packetLoss);

    ViewState GetViewState() const { return m_viewState; }
    void SetViewState(ViewState state) { m_viewState = state; }

    std::function<void(bool)> OnToggleOSFullscreen;

private:
    bool m_isInCall = false;
    ViewState m_viewState = ViewState::Inline;
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
