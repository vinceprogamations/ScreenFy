#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#include "Sidebar.h"
#include "ChatPanel.h"
#include "OverlayCall.h"
#include "SettingsPanel.h"
#include "ConnectionOverlay.h"

class AppWindow {
public:
    AppWindow();
    ~AppWindow();

    bool Initialize(int width, int height, const std::string& title);
    void SetD3D11(struct ID3D11Device* device, struct ID3D11DeviceContext* context);
    bool ProcessMessages();
    void RenderUI();

    HWND GetHWND() const { return m_hWnd; }
    void ToggleFullscreen(bool enable);

private:
    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void RenderIncomingCallModal();
    void RenderTutorialModal();

    HWND m_hWnd = nullptr;

    Sidebar m_sidebar;
    ChatPanel m_chatPanel;
    OverlayCall m_overlayCall;
    SettingsPanel m_settingsPanel;
    ConnectionOverlay m_connectionOverlay;

    // Estado de Configurações e Tutorial
    bool m_showSettings = false;
    bool m_showTutorial = false;

    // Estado da Chamada Recebida
    bool m_incomingCall = false;
    std::string m_incomingCaller;
    std::string m_incomingCallerIp;
    int m_incomingVideoPort = 50000;

    // Estado de Tela Cheia
    WINDOWPLACEMENT m_wpPrev = { sizeof(m_wpPrev) };
    bool m_isFullscreen = false;
};
