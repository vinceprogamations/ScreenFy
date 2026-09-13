#include "SettingsPanel.h"
#include "ThemeDiscord.h"
#include "TextureLoader.h"
#include "../utils/Logger.h"
#include "../capture/ScreenPreviewer.h"
#include "../encoder/UniversalVideoEncoder.h"
#include "../identity/FriendManager.h"
#include <imgui.h>
#include <cstring>
#include <algorithm>
#include <shellapi.h>
#include <filesystem>

SettingsPanel::SettingsPanel() = default;

void SettingsPanel::RefreshHardwareList() {
    m_monitors = SettingsManager::GetAvailableMonitors();
    m_microphones = SettingsManager::GetAvailableMicrophones();

    if (m_currentSettings.SelectedMonitorIndex < 0 || 
        m_currentSettings.SelectedMonitorIndex >= static_cast<int>(m_monitors.size())) {
        m_currentSettings.SelectedMonitorIndex = 0;
    }

    m_selectedMicIdx = 0;
    for (size_t i = 0; i < m_microphones.size(); ++i) {
        if (m_microphones[i] == m_currentSettings.SelectedMicrophone) {
            m_selectedMicIdx = static_cast<int>(i);
            break;
        }
    }
}

void SettingsPanel::Render(bool* pOpen) {
    if (!pOpen || !(*pOpen)) {
        m_loaded = false;
        return;
    }

    if (!m_loaded) {
        m_currentSettings = SettingsManager::Get().GetSettings();
        strncpy_s(m_nicknameBuffer, sizeof(m_nicknameBuffer), m_currentSettings.Nickname.c_str(), _TRUNCATE);

        RefreshHardwareList();

        // Resolução
        if (m_currentSettings.DefaultResolutionH == 2160) m_selectedResolutionIdx = 0;
        else if (m_currentSettings.DefaultResolutionH == 1440) m_selectedResolutionIdx = 1;
        else if (m_currentSettings.DefaultResolutionH == 1080) m_selectedResolutionIdx = 2;
        else if (m_currentSettings.DefaultResolutionH == 720) m_selectedResolutionIdx = 3;
        else m_selectedResolutionIdx = 2;

        // FPS
        if (m_currentSettings.DefaultFPS == 144) m_selectedFpsIdx = 2;
        else if (m_currentSettings.DefaultFPS == 120) m_selectedFpsIdx = 1;
        else m_selectedFpsIdx = 0; // 60 FPS

        m_loaded = true;
    }

    // Tecla ESC para fechar
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        *pOpen = false;
        m_loaded = false;
        return;
    }

    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(displaySize);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ThemeDiscord::COLOR_SIDEBAR_BG);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | 
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::Begin("##SettingsOverlay", pOpen, flags)) {
        float sidebarWidth = 240.0f;
        float contentWidth = displaySize.x - sidebarWidth;

        RenderSidebar(sidebarWidth, displaySize.y, pOpen);

        ImGui::SameLine();

        RenderContent(contentWidth, displaySize.y, pOpen);
    }
    ImGui::End();
    ImGui::PopStyleColor();
}

void SettingsPanel::RenderSidebar(float width, float height, bool* pOpen) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(23, 24, 27, 255));
    ImGui::BeginChild("##SettingsSidebar", ImVec2(width, height), false, ImGuiWindowFlags_NoScrollbar);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::SetCursorPosX(24);
    if (ThemeDiscord::FontSmall) ImGui::PushFont(ThemeDiscord::FontSmall);
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "CONFIGURACOES DE UTILIZADOR");
    if (ThemeDiscord::FontSmall) ImGui::PopFont();
    ImGui::Spacing();

    auto renderTabButton = [this, dl](const char* label, Tab tab) {
        bool isSelected = (m_currentTab == tab);
        ImVec2 startPos = ImGui::GetCursorScreenPos();
        ImVec2 btnSize(196, 38);
        ImVec2 endPos = ImVec2(startPos.x + btnSize.x, startPos.y + btnSize.y);

        bool isHovered = ImGui::IsMouseHoveringRect(startPos, endPos);

        ImGui::SetCursorPosX(20);
        if (ImGui::InvisibleButton(label, btnSize)) {
            m_currentTab = tab;
        }

        if (isSelected) {
            dl->AddRectFilled(startPos, endPos, IM_COL32(53, 55, 60, 255), 6.0f);
            dl->AddRectFilled(ImVec2(startPos.x + 2, startPos.y + 8), ImVec2(startPos.x + 5, startPos.y + 30), IM_COL32(88, 101, 242, 255), 2.0f);
        } else if (isHovered) {
            dl->AddRectFilled(startPos, endPos, IM_COL32(40, 42, 47, 180), 6.0f);
        }

        ImFont* font = isSelected ? (ThemeDiscord::FontBold ? ThemeDiscord::FontBold : ThemeDiscord::FontRegular) : ThemeDiscord::FontRegular;
        ImU32 textCol = isSelected ? IM_COL32(245, 246, 248, 255) : IM_COL32(148, 155, 164, 255);
        if (font) {
            dl->AddText(font, 15.0f, ImVec2(startPos.x + 16, startPos.y + 10), textCol, label);
        }
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
    };

    renderTabButton("Minha Conta", Tab::MyAccount);
    renderTabButton("Voz e Video", Tab::VoiceVideo);
    renderTabButton("Transmissao", Tab::Streaming);
    renderTabButton("Rede & Otimizacao", Tab::Network);
    renderTabButton("Diagnostico & Logs", Tab::Diagnostics);

    ImGui::Spacing();
    ImGui::SetCursorPosX(20);
    ImGui::Separator();
    ImGui::Spacing();

    // Botão ESC Fechar na barra lateral
    ImGui::SetCursorPosX(20);
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(35, 36, 40, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_RED_HOV);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeDiscord::COLOR_RED_ACT);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    if (ImGui::Button("ESC   Fechar", ImVec2(196, 36))) {
        *pOpen = false;
        m_loaded = false;
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void SettingsPanel::RenderContent(float width, float height, bool* pOpen) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ThemeDiscord::COLOR_WINDOW_BG);
    ImGui::BeginChild("##SettingsContent", ImVec2(width, height), false);

    // Botão circular Fechar ESC no canto superior direito estilo Discord
    float escBtnX = width - 84.0f;
    ImGui::SetCursorPos(ImVec2(escBtnX, 28));

    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(35, 36, 40, 200));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_RED_HOV);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeDiscord::COLOR_RED_ACT);
    ImGui::PushStyleColor(ImGuiCol_Border, ThemeDiscord::COLOR_BORDER_SUBTLE);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 18.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    if (ImGui::Button("X##CloseSettings", ImVec2(36, 36))) {
        *pOpen = false;
        m_loaded = false;
    }
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);

    ImGui::SetCursorPos(ImVec2(escBtnX + 8, 68));
    if (ThemeDiscord::FontSmall) ImGui::PushFont(ThemeDiscord::FontSmall);
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "ESC");
    if (ThemeDiscord::FontSmall) ImGui::PopFont();

    // Conteúdo principal centralizado e espaçado
    ImGui::SetCursorPos(ImVec2(48, 30));
    ImGui::BeginChild("##SettingsInnerContent", ImVec2(width - 150.0f, height - 50.0f), false);

    switch (m_currentTab) {
        case Tab::MyAccount:
            RenderMyAccount();
            break;
        case Tab::VoiceVideo:
            RenderVoiceVideo();
            break;
        case Tab::Streaming:
            RenderStreaming();
            break;
        case Tab::Network:
            RenderNetwork();
            break;
        case Tab::Diagnostics:
            RenderDiagnostics();
            break;
    }

    ImGui::EndChild();

    ImGui::EndChild();
    ImGui::PopStyleColor();
}

void SettingsPanel::RenderMyAccount() {
    if (ThemeDiscord::FontTitle) ImGui::PushFont(ThemeDiscord::FontTitle);
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "Minha Conta");
    if (ThemeDiscord::FontTitle) ImGui::PopFont();

    ImGui::Spacing();
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "Gerencie o seu perfil de transmissao, identidade P2P e codigo de conexao.");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Card do Perfil
    ImVec2 cPos = ImGui::GetCursorScreenPos();
    float cWidth = 620.0f;
    float cHeight = 310.0f;
    ImVec2 cEnd = ImVec2(cPos.x + cWidth, cPos.y + cHeight);
    ThemeDiscord::DrawCard(dl, cPos, cEnd, IM_COL32(43, 45, 49, 255), 10.0f, IM_COL32(56, 58, 64, 255), 1.0f);

    // Carregar Avatar customizado se configurado
    ID3D11ShaderResourceView* avatarSrv = TextureLoader::Get().LoadTextureFromFile(m_currentSettings.AvatarPath);
    ImVec2 avPos = ImVec2(cPos.x + 46, cPos.y + 50);
    ThemeDiscord::DrawAvatar(dl, avPos, 30.0f, m_currentSettings.Nickname, true, true, avatarSrv, m_currentSettings.Status);

    if (ThemeDiscord::FontBold) ImGui::PushFont(ThemeDiscord::FontBold);
    dl->AddText(ImVec2(cPos.x + 94, cPos.y + 28), IM_COL32(245, 246, 248, 255), m_currentSettings.Nickname.c_str());
    if (ThemeDiscord::FontBold) ImGui::PopFont();

    auto myProf = FriendManager::Instance().GetMyProfile();
    std::string ipStr = "Radmin VPN: " + myProf.RadminIP;
    dl->AddText(ImVec2(cPos.x + 94, cPos.y + 52), IM_COL32(35, 165, 90, 255), ipStr.c_str());

    // Botão de Alterar Foto e Remover Foto
    ImGui::SetCursorPos(ImVec2(400, 32));
    ImGui::PushStyleColor(ImGuiCol_Button, ThemeDiscord::COLOR_BLURPLE);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_BLURPLE_HOV);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeDiscord::COLOR_BLURPLE_ACT);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    if (ImGui::Button("📷 Alterar Foto", ImVec2(120, 32))) {
        std::string newAvatar = TextureLoader::OpenImageFileDialog();
        if (!newAvatar.empty()) {
            m_currentSettings.AvatarPath = newAvatar;
            SettingsManager::Get().UpdateSettings(m_currentSettings);
            LOG_INFO("PROFILE", "Foto de perfil atualizada: " + newAvatar);
        }
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Escolha uma imagem PNG, JPG ou BMP do seu computador");

    if (!m_currentSettings.AvatarPath.empty()) {
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(35, 36, 40, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_RED_HOV);
        if (ImGui::Button("Remover", ImVec2(75, 32))) {
            m_currentSettings.AvatarPath = "";
            SettingsManager::Get().UpdateSettings(m_currentSettings);
        }
        ImGui::PopStyleColor(2);
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    dl->AddLine(ImVec2(cPos.x + 20, cPos.y + 90), ImVec2(cEnd.x - 20, cPos.y + 90), IM_COL32(56, 58, 64, 255));

    // Seletor de Status (Online, Ausente, Ocupado, Invisível)
    ImGui::SetCursorPos(ImVec2(24, 102));
    if (ThemeDiscord::FontSmall) ImGui::PushFont(ThemeDiscord::FontSmall);
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "SEU ESTADO DE PRESENÇA:");
    if (ThemeDiscord::FontSmall) ImGui::PopFont();

    ImGui::SetCursorPos(ImVec2(24, 122));
    auto renderStatusBtn = [&](const char* label, int stCode, ImU32 dotCol) {
        bool isCur = (m_currentSettings.Status == stCode);
        if (isCur) {
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(50, 53, 60, 255));
            ImGui::PushStyleColor(ImGuiCol_Border, dotCol);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(32, 34, 37, 255));
            ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        }
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        if (ImGui::Button(label, ImVec2(136, 30))) {
            m_currentSettings.Status = stCode;
            SettingsManager::Get().UpdateSettings(m_currentSettings);
        }
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
        ImGui::SameLine();
    };

    renderStatusBtn("🟢 Online", 0, IM_COL32(35, 165, 90, 255));
    renderStatusBtn("🟡 Ausente", 1, IM_COL32(250, 166, 26, 255));
    renderStatusBtn("🔴 Ocupado", 2, IM_COL32(237, 66, 69, 255));
    renderStatusBtn("⚪ Invisível", 3, IM_COL32(128, 132, 142, 255));
    ImGui::NewLine();

    // Input do Nickname
    ImGui::SetCursorPos(ImVec2(24, 164));
    if (ThemeDiscord::FontSmall) ImGui::PushFont(ThemeDiscord::FontSmall);
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "ALTERAR NICKNAME (NOME VISÍVEL):");
    if (ThemeDiscord::FontSmall) ImGui::PopFont();

    ImGui::SetCursorPos(ImVec2(24, 186));
    ImGui::SetNextItemWidth(380);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(22, 23, 25, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    if (ImGui::InputText("##settings_nickname", m_nicknameBuffer, sizeof(m_nicknameBuffer))) {
        m_currentSettings.Nickname = m_nicknameBuffer;
        SettingsManager::Get().UpdateSettings(m_currentSettings);
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ThemeDiscord::COLOR_GREEN);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_GREEN_HOV);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    if (ImGui::Button("Guardar Nome", ImVec2(120, 32))) {
        m_currentSettings.Nickname = m_nicknameBuffer;
        SettingsManager::Get().UpdateSettings(m_currentSettings);
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);

    // Friend Code com Animação de Cópia
    std::string friendCode = FriendManager::Instance().GetMyFriendCode();
    ImGui::SetCursorPos(ImVec2(24, 228));
    if (ThemeDiscord::FontSmall) ImGui::PushFont(ThemeDiscord::FontSmall);
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "O SEU FRIEND CODE ATUAL (Envie para os seus amigos):");
    if (ThemeDiscord::FontSmall) ImGui::PopFont();

    ImGui::SetCursorPos(ImVec2(24, 248));
    char codeBuf[512];
    strncpy_s(codeBuf, sizeof(codeBuf), friendCode.c_str(), _TRUNCATE);
    ImGui::SetNextItemWidth(380);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(22, 23, 25, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    ImGui::InputText("##my_code_display", codeBuf, sizeof(codeBuf), ImGuiInputTextFlags_ReadOnly);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    ImGui::SameLine();
    if (m_copyFeedbackTimer > 0.0f) {
        m_copyFeedbackTimer -= ImGui::GetIO().DeltaTime;
        ImGui::PushStyleColor(ImGuiCol_Button, ThemeDiscord::COLOR_GREEN);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_GREEN);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        ImGui::Button("✓ Copiado!", ImVec2(120, 32));
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ThemeDiscord::COLOR_BLURPLE);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_BLURPLE_HOV);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        if (ImGui::Button("Copiar Código", ImVec2(120, 32))) {
            ImGui::SetClipboardText(friendCode.c_str());
            m_copyFeedbackTimer = 2.5f;
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
    }

    ImGui::SetCursorPosY(cHeight + 35);
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "O seu perfil e foto sao guardados automaticamente de forma permanente.");
}

void SettingsPanel::RenderVoiceVideo() {
    if (ThemeDiscord::FontTitle) ImGui::PushFont(ThemeDiscord::FontTitle);
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "Voz e Video");
    if (ThemeDiscord::FontTitle) ImGui::PopFont();

    ImGui::Spacing();
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "Configure os seus dispositivos de entrada de audio e o ecra selecionado para transmissao.");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Dispositivo de Entrada de Áudio
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "Dispositivo de Entrada (Microfone)");
    ImGui::Spacing();

    if (!m_microphones.empty()) {
        std::vector<const char*> micItems;
        for (const auto& mic : m_microphones) {
            micItems.push_back(mic.c_str());
        }

        ImGui::SetNextItemWidth(460);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        if (ImGui::Combo("##microphones_combo", &m_selectedMicIdx, micItems.data(), static_cast<int>(micItems.size()))) {
            if (m_selectedMicIdx >= 0 && m_selectedMicIdx < static_cast<int>(m_microphones.size())) {
                m_currentSettings.SelectedMicrophone = m_microphones[m_selectedMicIdx];
                SettingsManager::Get().UpdateSettings(m_currentSettings);
            }
        }
        ImGui::PopStyleVar();
    }
    ImGui::Spacing();

    // Volume do Microfone
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "Volume do Microfone");
    ImGui::SetNextItemWidth(460);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    if (ImGui::SliderInt("##mic_volume", &m_currentSettings.MicVolume, 0, 100, "%d%%")) {
        SettingsManager::Get().UpdateSettings(m_currentSettings);
    }
    ImGui::PopStyleVar();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Monitor Alvo para Captura
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "Monitor Alvo para Transmissao");
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "Selecione qual o ecra que sera capturado em resolucao nativa:");
    ImGui::Spacing();

    if (!m_monitors.empty()) {
        std::vector<const char*> monitorItems;
        for (const auto& mon : m_monitors) {
            monitorItems.push_back(mon.c_str());
        }

        ImGui::SetNextItemWidth(460);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        if (ImGui::Combo("##monitors_combo", &m_currentSettings.SelectedMonitorIndex, monitorItems.data(), static_cast<int>(monitorItems.size()))) {
            SettingsManager::Get().UpdateSettings(m_currentSettings);
        }
        ImGui::PopStyleVar();
    }

    ImGui::Spacing();
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "Teste de Pre-visualizacao do Ecra");
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "Teste a captura do monitor em tempo real para confirmar a selecao correta:");
    ImGui::Spacing();

    bool isTesting = ScreenPreviewer::Get().IsTesting();
    if (isTesting) {
        ImGui::Image(reinterpret_cast<ImTextureID>(ScreenPreviewer::Get().GetSRV()), ImVec2(320, 180));
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::TextColored(ThemeDiscord::COLOR_GREEN, "● Preview Ativo (%.0f FPS)", ScreenPreviewer::Get().GetCurrentFPS());
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button, ThemeDiscord::COLOR_RED);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_RED_HOV);
        if (ImGui::Button("⏹ Parar Teste", ImVec2(140, 36))) {
            ScreenPreviewer::Get().StopTest();
        }
        ImGui::PopStyleColor(2);
        ImGui::EndGroup();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ThemeDiscord::COLOR_BLURPLE);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_BLURPLE_HOV);
        if (ImGui::Button("👁 Testar Monitor Selecionado", ImVec2(240, 36))) {
            ScreenPreviewer::Get().StartTest();
        }
        ImGui::PopStyleColor(2);
    }
}

void SettingsPanel::RenderStreaming() {
    if (ThemeDiscord::FontTitle) ImGui::PushFont(ThemeDiscord::FontTitle);
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "Transmissao e Desempenho");
    if (ThemeDiscord::FontTitle) ImGui::PopFont();

    ImGui::Spacing();
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "Ajuste os padroes de resolucao, framerate e compatibilidade do motor grafico.");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Resolução Base
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "Resolucao Padrao de Transmissao");
    const char* resolutions[] = {
        "4K Ultra HD (3840x2160) - Maxima Nitidez",
        "2K Quad HD (2560x1440) - Equilibrado",
        "Full HD (1920x1080) - Alta Compatibilidade",
        "HD (1280x720) - Menor Consumo"
    };

    ImGui::SetNextItemWidth(460);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    if (ImGui::Combo("##res_combo", &m_selectedResolutionIdx, resolutions, IM_ARRAYSIZE(resolutions))) {
        if (m_selectedResolutionIdx == 0) m_currentSettings.DefaultResolutionH = 2160;
        else if (m_selectedResolutionIdx == 1) m_currentSettings.DefaultResolutionH = 1440;
        else if (m_selectedResolutionIdx == 2) m_currentSettings.DefaultResolutionH = 1080;
        else if (m_selectedResolutionIdx == 3) m_currentSettings.DefaultResolutionH = 720;
        SettingsManager::Get().UpdateSettings(m_currentSettings);
    }
    ImGui::PopStyleVar();
    ImGui::Spacing();

    // FPS
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "Taxa de Atualizacao (FPS)");
    const char* fpsOptions[] = {
        "60 FPS (Padrao Fluido)",
        "120 FPS (Alta Fluidez)",
        "144 FPS (Ultra Competitivo)"
    };

    ImGui::SetNextItemWidth(460);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    if (ImGui::Combo("##fps_combo", &m_selectedFpsIdx, fpsOptions, IM_ARRAYSIZE(fpsOptions))) {
        if (m_selectedFpsIdx == 0) m_currentSettings.DefaultFPS = 60;
        else if (m_selectedFpsIdx == 1) m_currentSettings.DefaultFPS = 120;
        else if (m_selectedFpsIdx == 2) m_currentSettings.DefaultFPS = 144;
        SettingsManager::Get().UpdateSettings(m_currentSettings);
    }
    ImGui::PopStyleVar();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Detecção e Informação de GPU / Encoder de Hardware (AMD / NVIDIA / Intel)
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "Aceleracao de Hardware de Video");
    ImGui::Spacing();

    ImVec2 gpuCardStart = ImGui::GetCursorScreenPos();
    ImVec2 gpuCardEnd = ImVec2(gpuCardStart.x + 460.0f, gpuCardStart.y + 70.0f);
    ThemeDiscord::DrawCard(ImGui::GetWindowDrawList(), gpuCardStart, gpuCardEnd, IM_COL32(32, 34, 38, 255), 8.0f, IM_COL32(50, 52, 58, 255), 1.0f);

    ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + 16, ImGui::GetCursorPosY() + 12));
    ImGui::BeginGroup();

    // Obter nome da GPU
    std::string gpuName = "Detectando GPU...";
    uint32_t vendorId = 0;
    auto vType = UniversalVideoEncoder::DetectGpuVendor(ScreenPreviewer::Get().GetDevice(), gpuName, vendorId);

    std::string encLabel = "Codificador Universal MFT (Direct3D 11)";
    ImU32 badgeCol = IM_COL32(88, 101, 242, 255);
    if (vType == GpuVendorType::AMD) {
        encLabel = "AMD Radeon VCE/VCN Hardware Encoder (Zero-Copy)";
        badgeCol = IM_COL32(237, 66, 69, 255); // Vermelho AMD
    } else if (vType == GpuVendorType::NVIDIA) {
        encLabel = "NVIDIA NVENC (HEVC Ultra-Low Latency)";
        badgeCol = IM_COL32(118, 185, 0, 255); // Verde NVIDIA
    } else if (vType == GpuVendorType::Intel) {
        encLabel = "Intel QuickSync Video Hardware (MFT)";
        badgeCol = IM_COL32(0, 113, 197, 255); // Azul Intel
    }

    if (ThemeDiscord::FontBold) ImGui::PushFont(ThemeDiscord::FontBold);
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "GPU: %s", gpuName.c_str());
    if (ThemeDiscord::FontBold) ImGui::PopFont();

    if (ThemeDiscord::FontSmall) ImGui::PushFont(ThemeDiscord::FontSmall);
    ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(badgeCol), "● %s", encLabel.c_str());
    if (ThemeDiscord::FontSmall) ImGui::PopFont();

    ImGui::EndGroup();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 18);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Fallback WGC
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "Compatibilidade de Captura de Janela");
    if (ImGui::Checkbox("Fallback automatico para Windows Graphics Capture (WGC)", &m_currentSettings.UseWGCFallback)) {
        SettingsManager::Get().UpdateSettings(m_currentSettings);
    }
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "Permite a comutacao automatica se a API DXGI Desktop Duplication falhar.");
}

void SettingsPanel::RenderNetwork() {
    if (ThemeDiscord::FontTitle) ImGui::PushFont(ThemeDiscord::FontTitle);
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "Rede & Otimizacao UDP");
    if (ThemeDiscord::FontTitle) ImGui::PopFont();

    ImGui::Spacing();
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "Configure a alocacao de largura de banda e protecao contra perda de pacotes.");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Bitrate Alvo
    int bitrateMbps = m_currentSettings.TargetBitrateKbps / 1000;
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "Bitrate de Codificacao Alvo");
    ImGui::SetNextItemWidth(460);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    if (ImGui::SliderInt("##bitrate_slider", &bitrateMbps, 10, 150, "%d Mbps")) {
        m_currentSettings.TargetBitrateKbps = bitrateMbps * 1000;
        SettingsManager::Get().UpdateSettings(m_currentSettings);
    }
    ImGui::PopStyleVar();
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "Recomendado para 4K a 60/120 FPS com NVENC HEVC: 85 Mbps.");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Redundância FEC Reed-Solomon
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "Redundancia FEC Reed-Solomon");
    ImGui::SetNextItemWidth(460);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    if (ImGui::SliderInt("##fec_slider", &m_currentSettings.FecRedundancyPercent, 0, 50, "%d%%")) {
        SettingsManager::Get().UpdateSettings(m_currentSettings);
    }
    ImGui::PopStyleVar();
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "Gera blocos de paridade GF(2^8) para reparar perda de pacotes e jitter sem retransmissoes.");
}

void SettingsPanel::RenderDiagnostics() {
    if (ThemeDiscord::FontTitle) ImGui::PushFont(ThemeDiscord::FontTitle);
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "Diagnóstico & Registos do Sistema");
    if (ThemeDiscord::FontTitle) ImGui::PopFont();

    ImGui::Spacing();
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "Acompanhe mensagens de erro, inicialização de hardware (DXGI, NVENC, WASAPI) e rede P2P em tempo real.");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Barra de Ferramentas de Diagnóstico
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

    // Botão Copiar Relatório
    ImGui::PushStyleColor(ImGuiCol_Button, ThemeDiscord::COLOR_BLURPLE);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_BLURPLE_HOV);
    if (ImGui::Button("📋 Copiar Diagnóstico Completo", ImVec2(220, 34))) {
        std::string diag = Logger::Get().GenerateDiagnosticReport();
        ImGui::SetClipboardText(diag.c_str());
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Copia as especificacoes do seu PC, adaptadores de video e os logs recentes para a area de transferencia");
    ImGui::PopStyleColor(2);

    ImGui::SameLine();

    // Botão Abrir Pasta de Logs
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(43, 45, 49, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(60, 63, 70, 255));
    if (ImGui::Button("📂 Abrir Pasta de Logs", ImVec2(170, 34))) {
        std::string logPath = Logger::Get().GetLogFilePath();
        std::filesystem::path dir = std::filesystem::path(logPath).parent_path();
        ShellExecuteA(nullptr, "open", dir.string().c_str(), nullptr, nullptr, SW_SHOW);
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine();

    // Botão Limpar
    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(35, 36, 40, 255));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_RED_HOV);
    if (ImGui::Button("🗑️ Limpar", ImVec2(90, 34))) {
        Logger::Get().ClearMemoryLogs();
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine();

    // Filtro de Nível
    const char* filterOptions[] = { "Todos os Níveis", "Apenas Erros", "Avisos e Erros", "Informações" };
    ImGui::SetNextItemWidth(160);
    ImGui::Combo("##log_filter", &m_logLevelFilter, filterOptions, IM_ARRAYSIZE(filterOptions));

    ImGui::PopStyleVar();

    ImGui::Spacing();

    // Viewport de Logs
    float listHeight = 360.0f;
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(18, 19, 22, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));
    ImGui::BeginChild("##DiagnosticsLogViewer", ImVec2(0, listHeight), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    auto logs = Logger::Get().GetRecentLogs(300);
    if (logs.empty()) {
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "Nenhum registo de log gerado ainda nesta sessao.");
    } else {
        for (const auto& entry : logs) {
            // Aplicar filtro
            if (m_logLevelFilter == 1 && entry.level != LogLevel::Error && entry.level != LogLevel::Fatal) continue;
            if (m_logLevelFilter == 2 && entry.level != LogLevel::Warn && entry.level != LogLevel::Error && entry.level != LogLevel::Fatal) continue;
            if (m_logLevelFilter == 3 && entry.level != LogLevel::Info) continue;

            auto t = std::chrono::system_clock::to_time_t(entry.timestamp);
            std::tm tmLog;
            localtime_s(&tmLog, &t);
            char timeStr[32];
            strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &tmLog);

            ImVec4 levelColor = ThemeDiscord::COLOR_TEXT_MUTED;
            const char* levelName = "INFO";
            switch (entry.level) {
                case LogLevel::Debug: levelColor = ImVec4(0.58f, 0.61f, 0.64f, 1.0f); levelName = "DBG"; break;
                case LogLevel::Info:  levelColor = ThemeDiscord::COLOR_GREEN; levelName = "INF"; break;
                case LogLevel::Warn:  levelColor = ImVec4(0.98f, 0.65f, 0.10f, 1.0f); levelName = "WRN"; break;
                case LogLevel::Error: levelColor = ThemeDiscord::COLOR_RED; levelName = "ERR"; break;
                case LogLevel::Fatal: levelColor = ImVec4(1.00f, 0.10f, 0.30f, 1.0f); levelName = "FTL"; break;
            }

            ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "[%s]", timeStr);
            ImGui::SameLine();
            ImGui::TextColored(levelColor, "[%s]", levelName);
            ImGui::SameLine();
            ImGui::TextColored(ThemeDiscord::COLOR_BLURPLE, "[%s]", entry.tag.c_str());
            ImGui::SameLine();
            ImGui::TextUnformatted(entry.message.c_str());
        }

        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 20.0f) {
            ImGui::SetScrollHereY(1.0f);
        }
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "Ficheiro local permanente: %s", Logger::Get().GetLogFilePath().c_str());
}
