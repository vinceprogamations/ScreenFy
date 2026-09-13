#include "AppWindow.h"
#include "ThemeDiscord.h"
#include "TextureLoader.h"
#include "../utils/Logger.h"
#include "../capture/ScreenPreviewer.h"
#include "../identity/FriendManager.h"
#include "../network/SignalingClient.h"
#include "../config/SettingsManager.h"
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <iostream>
#include <algorithm>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

AppWindow::AppWindow() = default;

AppWindow::~AppWindow() {
    SignalingClient::Instance().Stop();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    if (m_hWnd) DestroyWindow(m_hWnd);
}

void AppWindow::SetD3D11(ID3D11Device* device, ID3D11DeviceContext* context) {
    ScreenPreviewer::Get().Initialize(device, context);
    TextureLoader::Get().Initialize(device, context);
}

LRESULT CALLBACK AppWindow::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        return true;

    if (uMsg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

bool AppWindow::Initialize(int width, int height, const std::string& title) {
    WNDCLASSEX wc = { 
        sizeof(WNDCLASSEX), CS_CLASSDC, WindowProc, 0L, 0L, 
        GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, 
        "ScreenShareClass", nullptr 
    };
    RegisterClassEx(&wc);
    
    m_hWnd = CreateWindow(
        wc.lpszClassName, title.c_str(), WS_OVERLAPPEDWINDOW, 
        100, 100, width, height, nullptr, nullptr, wc.hInstance, nullptr
    );
    if (!m_hWnd) return false;

    // Lote 7: Detetar DPI e aplicar Auto-Scaling
    UINT dpi = 96;
    HMODULE hUser32 = GetModuleHandleA("user32.dll");
    if (hUser32) {
        typedef UINT(WINAPI* PFN_GetDpiForWindow)(HWND);
        auto pfn = reinterpret_cast<PFN_GetDpiForWindow>(GetProcAddress(hUser32, "GetDpiForWindow"));
        if (pfn) {
            dpi = pfn(m_hWnd);
        }
    }
    if (dpi == 0) {
        HDC hdc = GetDC(m_hWnd);
        dpi = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(m_hWnd, hdc);
    }
    float dpiScale = (dpi > 0) ? (static_cast<float>(dpi) / 96.0f) : 1.0f;
    if (dpiScale <= 0.0f) dpiScale = 1.0f;

    int scaledWidth = static_cast<int>(width * dpiScale);
    int scaledHeight = static_cast<int>(height * dpiScale);
    SetWindowPos(m_hWnd, nullptr, 0, 0, scaledWidth, scaledHeight, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

    ShowWindow(m_hWnd, SW_SHOWDEFAULT);
    UpdateWindow(m_hWnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.FontGlobalScale = dpiScale;

    // 1. Aplicar Estilo Discord e Fontes
    ThemeDiscord::ApplyStyle();
    ThemeDiscord::LoadFonts(io);

    ImGui_ImplWin32_Init(m_hWnd);

    // 2. Inicializar Gestão de Configurações e Identidade
    SettingsManager::Get().Initialize();
    FriendManager::Instance().Initialize();

    // Callbacks para abrir Configurações e Tutorial
    m_sidebar.OnOpenSettings = [this]() {
        m_showSettings = true;
    };
    m_sidebar.OnOpenTutorial = [this]() {
        m_showTutorial = true;
    };
    m_chatPanel.OnOpenTutorial = [this]() {
        m_showTutorial = true;
    };

    // 3. Inicializar Servidor e Cliente de Sinalização TCP
    SignalingClient::Instance().Start(49152);

    // Callbacks do Signaling
    SignalingClient::Instance().OnCallRequest = [this](const std::string& caller, const std::string& callerIp, int videoPort) {
        m_incomingCall = true;
        m_incomingCaller = caller;
        m_incomingCallerIp = callerIp;
        m_incomingVideoPort = videoPort;
        m_chatPanel.AddLog("Chamada recebida de " + caller + " (" + callerIp + ")");
    };

    SignalingClient::Instance().OnCallAccepted = [this](const std::string& targetIp) {
        m_chatPanel.AddLog("A chamada foi aceite por " + targetIp);
        m_overlayCall.SetInCall(true, true, "Amigo", targetIp);
    };

    SignalingClient::Instance().OnCallRejected = [this](const std::string& targetIp) {
        m_chatPanel.AddLog("A chamada foi recusada por " + targetIp);
        m_overlayCall.SetInCall(false, false, "", "");
    };

    SignalingClient::Instance().OnCallEnded = [this](const std::string& targetIp) {
        m_chatPanel.AddLog("A chamada foi encerrada por " + targetIp);
        m_overlayCall.SetInCall(false, false, "", "");
    };

    // Callback da UI para Iniciar Chamada
    m_chatPanel.OnStartCall = [this](const UserProfile& target, int resolution, int fps) {
        (void)resolution;
        (void)fps;
        auto myProf = FriendManager::Instance().GetMyProfile();
        SignalingClient::Instance().SendCallRequest(target.RadminIP, myProf.Nickname, 50000);
        m_overlayCall.SetInCall(true, true, target.Nickname, target.RadminIP);
        m_chatPanel.AddLog("A enviar pedido de partilha para " + target.Nickname + " (" + target.RadminIP + ")...");
    };

    // Callback para Desligar
    m_overlayCall.OnEndCall = [this]() {
        auto friendOpt = m_sidebar.GetSelectedFriend();
        if (friendOpt) {
            SignalingClient::Instance().SendCallEnd(friendOpt->RadminIP);
        }
        m_chatPanel.AddLog("Sessao de partilha terminada.");
    };

    return true;
}

bool AppWindow::ProcessMessages() {
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_QUIT) return false;
    }
    return true;
}

void AppWindow::RenderUI() {
    ScreenPreviewer::Get().Update();

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImVec2 displaySize = ImGui::GetIO().DisplaySize;

    if (m_overlayCall.IsInCall()) {
        // Modo de Transmissão / Visualização Fullscreen
        m_overlayCall.Render(displaySize.x, displaySize.y);
    } else {
        // Modo Padrão Discord: Múltiplas Colunas com Layout Responsivo
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(displaySize);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ThemeDiscord::COLOR_WINDOW_BG);
        ImGui::Begin("ScreenShare4KMain", nullptr, 
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | 
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);

        float sidebarWidth = displaySize.x * 0.22f;
        if (sidebarWidth < 240.0f) sidebarWidth = 240.0f;
        if (sidebarWidth > 320.0f) sidebarWidth = 320.0f;

        m_sidebar.Render(sidebarWidth, displaySize.y);

        ImGui::SameLine(0, 0);

        float remainingWidth = displaySize.x - sidebarWidth;
        if (remainingWidth < 400.0f) remainingWidth = 400.0f;
        m_chatPanel.Render(m_sidebar.GetSelectedFriend(), remainingWidth, displaySize.y);

        ImGui::End();
        ImGui::PopStyleColor();

        // Se o painel de configuracoes estiver aberto, renderiza sobreposto em ecra inteiro
        if (m_showSettings) {
            m_settingsPanel.Render(&m_showSettings);
        }
    }

    RenderIncomingCallModal();
    RenderTutorialModal();

    ImGui::Render();
}

void AppWindow::RenderIncomingCallModal() {
    if (m_incomingCall) {
        ImGui::OpenPopup("Incoming Call##Modal");
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(460, 210));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24, 20));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ThemeDiscord::COLOR_CARD_BG);
    ImGui::PushStyleColor(ImGuiCol_Border, ThemeDiscord::COLOR_BORDER_SUBTLE);

    if (ImGui::BeginPopupModal("Incoming Call##Modal", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        if (ThemeDiscord::FontHeader) ImGui::PushFont(ThemeDiscord::FontHeader);
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "Chamada Recebida!");
        if (ThemeDiscord::FontHeader) ImGui::PopFont();

        ImGui::Spacing();
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "%s (%s)", m_incomingCaller.c_str(), m_incomingCallerIp.c_str());
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "Deseja aceitar a transmissao de ecra direta em 4K?");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 260);

        // Botão Recusar (Vermelho)
        ImGui::PushStyleColor(ImGuiCol_Button, ThemeDiscord::COLOR_RED);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_RED_HOV);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeDiscord::COLOR_RED_ACT);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        if (ImGui::Button("Recusar", ImVec2(115, 36))) {
            SignalingClient::Instance().SendCallReject(m_incomingCallerIp);
            m_incomingCall = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();

        // Botão Aceitar (Verde)
        ImGui::PushStyleColor(ImGuiCol_Button, ThemeDiscord::COLOR_GREEN);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_GREEN_HOV);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeDiscord::COLOR_GREEN_ACT);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        if (ImGui::Button("Aceitar", ImVec2(115, 36))) {
            SignalingClient::Instance().SendCallAccept(m_incomingCallerIp);
            m_overlayCall.SetInCall(true, false, m_incomingCaller, m_incomingCallerIp);
            m_incomingCall = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        ImGui::EndPopup();
    }

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

void AppWindow::RenderTutorialModal() {
    if (m_showTutorial) {
        ImGui::OpenPopup("Como Usar o ScreenFy##Modal");
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(620, 520));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24, 20));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ThemeDiscord::COLOR_CARD_BG);
    ImGui::PushStyleColor(ImGuiCol_Border, ThemeDiscord::COLOR_BORDER_SUBTLE);

    if (ImGui::BeginPopupModal("Como Usar o ScreenFy##Modal", &m_showTutorial, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        if (ThemeDiscord::FontHeader) ImGui::PushFont(ThemeDiscord::FontHeader);
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "Como Usar o ScreenFy (Guia Rapido)");
        if (ThemeDiscord::FontHeader) ImGui::PopFont();

        ImGui::Spacing();
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "Envie este passo-a-passo aos seus amigos para se conectarem em segundos:");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::BeginChild("TutorialContentScroll", ImVec2(0, 310), false);

        auto renderStep = [](const char* num, const char* title, const char* desc) {
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 cur = ImGui::GetCursorScreenPos();
            dl->AddCircleFilled(ImVec2(cur.x + 14, cur.y + 14), 13.0f, IM_COL32(88, 101, 242, 255));
            ImFont* f = ThemeDiscord::FontBold ? ThemeDiscord::FontBold : ThemeDiscord::FontRegular;
            if (f) dl->AddText(f, 13.0f, ImVec2(cur.x + 10, cur.y + 6), IM_COL32(255, 255, 255, 255), num);

            ImGui::SetCursorPosX(36);
            if (ThemeDiscord::FontBold) ImGui::PushFont(ThemeDiscord::FontBold);
            ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "%s", title);
            if (ThemeDiscord::FontBold) ImGui::PopFont();

            ImGui::SetCursorPosX(36);
            ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "%s", desc);
            ImGui::Spacing();
            ImGui::Spacing();
        };

        renderStep("1", "Instalar e Ligar o Radmin VPN", 
            "Ambos precisam do Radmin VPN instalado (gratuito em radmin-vpn.com).\nCrie uma rede (ex: 'Amigos4K') ou entre na mesma rede. O seu IP sera 26.x.x.x.");

        renderStep("2", "Abrir o ScreenShare.exe", 
            "Execute o ScreenShare.exe. O programa detetara automaticamente o seu IP da Radmin VPN e ficara pronto para conexao.");

        renderStep("3", "Trocar o Friend Code", 
            "No ecra inicial, clique em 'Copiar Codigo'. Envie para o seu amigo no Discord ou WhatsApp.\nO seu colega clica em '+ Adicionar Amigo' na barra lateral e cola o seu codigo!");

        renderStep("4", "Testar e Iniciar a Partilha 4K", 
            "Clique no botao 'Testar Ecra (Preview)' para conferir a imagem em tempo real.\nQuando o seu amigo estiver online, clique em 'INICIAR TRANSMISSAO DE ECRA'!\nEle recebera um aviso para 'Aceitar' e o stream 4K comecara imediatamente.");

        ImGui::EndChild();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Botão Copiar Texto do Tutorial
        static bool s_copiedTut = false;
        static float s_copiedTutTimer = 0.0f;

        ImGui::PushStyleColor(ImGuiCol_Button, ThemeDiscord::COLOR_BLURPLE);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_BLURPLE_HOV);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        if (ImGui::Button("Copiar Tutorial para Enviar aos Amigos", ImVec2(340, 36))) {
            const char* fullText = 
                "=== GUIA RAPIDO: COMO USAR O SCREENFY 4K ===\n\n"
                "1. Baixe e abra o Radmin VPN (radmin-vpn.com) e entre na mesma rede que eu (IP 26.x.x.x).\n"
                "2. Abra o ScreenShare.exe.\n"
                "3. No ScreenFy, clique em '+ Adicionar Amigo' na barra lateral e cole o meu Friend Code.\n"
                "4. Quando eu aparecer na sua lista (ou voce na minha), clique em 'INICIAR TRANSMISSAO DE ECRA'.\n"
                "5. Uma janela pop-up surgira para 'Aceitar' e comecara o streaming em 4K ultra-fluido com som do PC!\n";
            ImGui::SetClipboardText(fullText);
            s_copiedTut = true;
            s_copiedTutTimer = 3.0f;
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);

        if (s_copiedTut) {
            ImGui::SameLine();
            ImGui::TextColored(ThemeDiscord::COLOR_GREEN, "✓ Copiado!");
            s_copiedTutTimer -= ImGui::GetIO().DeltaTime;
            if (s_copiedTutTimer <= 0.0f) s_copiedTut = false;
        }

        ImGui::SameLine(ImGui::GetWindowWidth() - 120);
        if (ImGui::Button("Fechar", ImVec2(90, 36))) {
            m_showTutorial = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}
