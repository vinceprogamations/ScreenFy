#include "ScreenTestModal.h"
#include "ThemeDiscord.h"
#include "../capture/ScreenPreviewer.h"
#include "../config/SettingsManager.h"
#include <imgui.h>
#include <iomanip>
#include <sstream>

ScreenTestModal::ScreenTestModal() = default;

void ScreenTestModal::Open() {
    m_isOpen = true;
    auto settings = SettingsManager::Get().GetSettings();
    ScreenPreviewer::Get().SetActiveMonitor(settings.SelectedMonitorIndex);
    ScreenPreviewer::Get().SetModalActive(true);
    ScreenPreviewer::Get().StartTest();
}

void ScreenTestModal::Close() {
    m_isOpen = false;
    ScreenPreviewer::Get().SetModalActive(false);
    ScreenPreviewer::Get().StopTest();
}

void ScreenTestModal::Render() {
    if (!m_isOpen) return;

    ImGui::OpenPopup("Estúdio de Teste de Ecrã##ScreenTestModal");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(820, 0), ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24, 20));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ThemeDiscord::COLOR_CARD_BG);
    ImGui::PushStyleColor(ImGuiCol_Border, ThemeDiscord::COLOR_BORDER_SUBTLE);

    bool isOpenLocal = m_isOpen;
    if (ImGui::BeginPopupModal("Estúdio de Teste de Ecrã##ScreenTestModal", &isOpenLocal,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)) {

        ImDrawList* dl = ImGui::GetWindowDrawList();

        // 1. Cabeçalho Principal
        if (ThemeDiscord::FontHeader) ImGui::PushFont(ThemeDiscord::FontHeader);
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "🖥️ Estúdio de Teste de Ecrã (Screen Test Studio)");
        if (ThemeDiscord::FontHeader) ImGui::PopFont();

        if (ThemeDiscord::FontSmall) ImGui::PushFont(ThemeDiscord::FontSmall);
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "Pré-visualização ultra-fluida em tempo real via Direct3D 11 Zero-Copy (DXGI)");
        if (ThemeDiscord::FontSmall) ImGui::PopFont();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 2. Seletor de Monitores (se houver mais de 1)
        int monCount = ScreenPreviewer::Get().GetMonitorCount();
        int activeMon = ScreenPreviewer::Get().GetActiveMonitorIndex();

        if (monCount > 1) {
            ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "SELECIONAR MONITOR PARA TESTAR:");
            ImGui::SameLine();
            for (int m = 0; m < monCount; ++m) {
                bool isSelected = (activeMon == m);
                if (isSelected) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ThemeDiscord::COLOR_BLURPLE);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_BLURPLE_HOV);
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(35, 36, 40, 255));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(50, 52, 58, 255));
                }
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

                std::string label = "Tela " + std::to_string(m + 1);
                if (ScreenPreviewer::Get().IsMonitorPrimary(m)) label += " [Principal]";

                if (ImGui::Button(label.c_str())) {
                    ScreenPreviewer::Get().SetActiveMonitor(m);
                    activeMon = m;
                    // Sincronizar com as configurações
                    auto s = SettingsManager::Get().GetSettings();
                    s.SelectedMonitorIndex = m;
                    SettingsManager::Get().UpdateSettings(s);
                }
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(2);
                ImGui::SameLine();
            }
            ImGui::NewLine();
            ImGui::Spacing();
        }

        // 3. Viewport de Vídeo 16:9 de Alta Resolução
        const float viewW = 772.0f;
        const float viewH = 434.0f; // 16:9 ratio

        ImVec2 vpStart = ImGui::GetCursorScreenPos();
        ImVec2 vpEnd = ImVec2(vpStart.x + viewW, vpStart.y + viewH);

        // Moldura do viewport
        dl->AddRectFilled(vpStart, vpEnd, IM_COL32(18, 19, 21, 255), 8.0f);
        dl->AddRect(vpStart, vpEnd, IM_COL32(60, 63, 72, 255), 8.0f, 0, 1.5f);

        // Renderizar textura D3D11 SRV
        ID3D11ShaderResourceView* srv = ScreenPreviewer::Get().GetSRV();
        if (srv) {
            ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY()));
            ImGui::Image(reinterpret_cast<ImTextureID>(srv), ImVec2(viewW, viewH));
        } else {
            ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + viewW * 0.4f, ImGui::GetCursorPosY() + viewH * 0.45f));
            ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "Aguardando sinal de vídeo...");
            ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX(), ImGui::GetCursorPosY() + viewH * 0.55f));
        }

        // 4. Badges de Telemetria no topo da imagem
        float curFps = ScreenPreviewer::Get().GetCurrentFPS();
        int natW = ScreenPreviewer::Get().GetMonitorNativeWidth(activeMon);
        int natH = ScreenPreviewer::Get().GetMonitorNativeHeight(activeMon);
        int hz = ScreenPreviewer::Get().GetMonitorRefreshRate(activeMon);
        float latency = ScreenPreviewer::Get().GetCaptureLatencyMs();

        auto drawHUDTag = [&](ImVec2 pos, const char* text, ImU32 bgCol, ImU32 textCol) {
            ImFont* f = ThemeDiscord::FontSmall ? ThemeDiscord::FontSmall : ThemeDiscord::FontRegular;
            ImVec2 txtSz = f ? f->CalcTextSizeA(13.0f, FLT_MAX, 0.0f, text) : ImVec2(80, 16);
            ImVec2 boxEnd = ImVec2(pos.x + txtSz.x + 16, pos.y + txtSz.y + 10);
            dl->AddRectFilled(pos, boxEnd, bgCol, 6.0f);
            if (f) dl->AddText(f, 13.0f, ImVec2(pos.x + 8, pos.y + 5), textCol, text);
            return boxEnd.x;
        };

        float badgeX = vpStart.x + 14.0f;
        float badgeY = vpStart.y + 14.0f;

        // Badge 1: FPS
        std::stringstream ssFps;
        ssFps << "🟢 AO VIVO: " << std::fixed << std::setprecision(1) << curFps << " FPS";
        badgeX = drawHUDTag(ImVec2(badgeX, badgeY), ssFps.str().c_str(), IM_COL32(0, 0, 0, 200), IM_COL32(35, 165, 90, 255)) + 8.0f;

        // Badge 2: Latência
        std::stringstream ssLat;
        ssLat << "⚡ LATÊNCIA: " << std::fixed << std::setprecision(1) << latency << " ms";
        badgeX = drawHUDTag(ImVec2(badgeX, badgeY), ssLat.str().c_str(), IM_COL32(0, 0, 0, 200), IM_COL32(0, 168, 252, 255)) + 8.0f;

        // Badge 3: Resolução e Hz
        std::string resStr = "🖥️ " + std::to_string(natW) + "x" + std::to_string(natH) + " @" + std::to_string(hz) + "Hz";
        drawHUDTag(ImVec2(badgeX, badgeY), resStr.c_str(), IM_COL32(0, 0, 0, 200), IM_COL32(245, 246, 248, 255));

        // Badge inferior: Pipeline
        std::string pipeStr = "PRODUÇÃO: Captura VRAM Zero-Copy • Renderizador Direct3D 11";
        drawHUDTag(ImVec2(vpStart.x + 14.0f, vpEnd.y - 32.0f), pipeStr.c_str(), IM_COL32(0, 0, 0, 200), IM_COL32(148, 155, 164, 255));

        ImGui::Spacing();
        ImGui::Spacing();

        // 5. Configurações de Qualidade da Transmissão (editáveis ao vivo)
        ImGui::Separator();
        ImGui::Spacing();

        if (ThemeDiscord::FontBold) ImGui::PushFont(ThemeDiscord::FontBold);
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "Configuração de Qualidade da Transmissão");
        if (ThemeDiscord::FontBold) ImGui::PopFont();
        ImGui::Spacing();

        auto currentSettings = SettingsManager::Get().GetSettings();
        bool settingsChanged = false;

        auto renderPill = [&](const char* label, bool active) {
            ImGui::PushID(label);
            if (active) {
                ImGui::PushStyleColor(ImGuiCol_Button, ThemeDiscord::COLOR_BLURPLE);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_BLURPLE_HOV);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeDiscord::COLOR_BLURPLE_ACT);
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f);
                ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(255, 255, 255, 200));
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(35, 36, 40, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(50, 52, 58, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(70, 72, 80, 255));
                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(180, 185, 195, 255));
                ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
                ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
            }
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 14.0f);
            bool clicked = ImGui::Button(label, ImVec2(0, 28));
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(5);
            ImGui::SameLine();
            ImGui::PopID();
            return clicked;
        };

        // Linha 1: Resolução
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "Resolução:");
        ImGui::SameLine(120);
        if (renderPill("720p", currentSettings.DefaultResolutionH == 720)) { currentSettings.DefaultResolutionH = 720; settingsChanged = true; }
        if (renderPill("1080p", currentSettings.DefaultResolutionH == 1080)) { currentSettings.DefaultResolutionH = 1080; settingsChanged = true; }
        if (renderPill("1440p (2K)", currentSettings.DefaultResolutionH == 1440)) { currentSettings.DefaultResolutionH = 1440; settingsChanged = true; }
        if (renderPill("2160p (4K)", currentSettings.DefaultResolutionH == 2160)) { currentSettings.DefaultResolutionH = 2160; settingsChanged = true; }
        ImGui::NewLine();

        // Linha 2: FPS
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "Framerate:");
        ImGui::SameLine(120);
        if (renderPill("30 FPS", currentSettings.DefaultFPS == 30)) { currentSettings.DefaultFPS = 30; settingsChanged = true; }
        if (renderPill("60 FPS", currentSettings.DefaultFPS == 60)) { currentSettings.DefaultFPS = 60; settingsChanged = true; }
        if (renderPill("120 FPS", currentSettings.DefaultFPS == 120)) { currentSettings.DefaultFPS = 120; settingsChanged = true; }
        ImGui::NewLine();

        // Linha 3: Bitrate de Áudio
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "Áudio:");
        ImGui::SameLine(120);
        if (renderPill("64 kbps", currentSettings.AudioBitrateKbps == 64)) { currentSettings.AudioBitrateKbps = 64; settingsChanged = true; }
        if (renderPill("128 kbps", currentSettings.AudioBitrateKbps == 128)) { currentSettings.AudioBitrateKbps = 128; settingsChanged = true; }
        if (renderPill("192 kbps", currentSettings.AudioBitrateKbps == 192)) { currentSettings.AudioBitrateKbps = 192; settingsChanged = true; }
        ImGui::NewLine();

        if (settingsChanged) {
            SettingsManager::Get().UpdateSettings(currentSettings);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 6. Rodapé: Informações + Botões
        {
            auto cfg = SettingsManager::Get().GetSettings();
            std::string info = "Config: " + std::to_string(cfg.DefaultResolutionH) + "p • " +
                               std::to_string(cfg.DefaultFPS) + " FPS • " +
                               std::to_string(cfg.AudioBitrateKbps) + " kbps áudio";
            if (ThemeDiscord::FontSmall) ImGui::PushFont(ThemeDiscord::FontSmall);
            ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "%s", info.c_str());
            if (ThemeDiscord::FontSmall) ImGui::PopFont();
        }

        ImGui::SameLine(ImGui::GetWindowWidth() - 360);

        // Botão Fechar Teste
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(43, 45, 49, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(60, 63, 70, 255));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        if (ImGui::Button("⏹ Fechar Teste", ImVec2(150, 38))) {
            Close();
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);

        ImGui::SameLine();

        // Botão Transmitir Agora
        ImGui::PushStyleColor(ImGuiCol_Button, ThemeDiscord::COLOR_BLURPLE);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_BLURPLE_HOV);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeDiscord::COLOR_BLURPLE_ACT);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        if (ImGui::Button("📺 Transmitir Agora", ImVec2(170, 38))) {
            Close();
            ImGui::CloseCurrentPopup();
            if (OnGoLiveRequested) {
                OnGoLiveRequested();
            }
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        ImGui::EndPopup();
    } else {
        Close();
    }

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}
