#pragma once
#include <string>
#include <vector>
#include "../config/SettingsManager.h"

class SettingsPanel {
public:
    enum class Tab {
        MyAccount,
        VoiceVideo,
        Streaming,
        Network,
        Diagnostics
    };

    SettingsPanel();
    ~SettingsPanel() = default;

    void Render(bool* pOpen);

private:
    void RefreshHardwareList();
    void RenderSidebar(float width, float height, bool* pOpen);
    void RenderContent(float width, float height, bool* pOpen);

    void RenderMyAccount();
    void RenderVoiceVideo();
    void RenderStreaming();
    void RenderNetwork();
    void RenderDiagnostics();

    Tab m_currentTab = Tab::MyAccount;
    AppSettings m_currentSettings;
    bool m_loaded = false;

    char m_nicknameBuffer[128] = "";
    float m_copyFeedbackTimer = 0.0f;

    std::vector<std::string> m_monitors;
    std::vector<std::string> m_microphones;

    int m_selectedResolutionIdx = 0; // 0: 4K, 1: 1440p, 2: 1080p, 3: 720p
    int m_selectedFpsIdx = 0;        // 0: 60 FPS, 1: 120 FPS, 2: 144 FPS
    int m_selectedMicIdx = 0;
    int m_logLevelFilter = 0;        // 0: Todos, 1: Erros, 2: Avisos, 3: Info
};
