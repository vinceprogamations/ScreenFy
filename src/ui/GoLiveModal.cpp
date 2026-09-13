#include "GoLiveModal.h"
#include "ThemeDiscord.h"
#include "../capture/ScreenPreviewer.h"
#include "../config/SettingsManager.h"
#include <imgui.h>

GoLiveModal::GoLiveModal() = default;

void GoLiveModal::Open(const UserProfile& targetFriend) {
    m_isOpen = true;
    m_targetFriend = targetFriend;
    m_selectedTab = 0;

    auto settings = SettingsManager::Get().GetSettings();
    m_selectedMonitorIdx = settings.SelectedMonitorIndex;

    // Mapear Resolução
    if (settings.DefaultResolutionH <= 720) m_selectedResIdx = 0;
    else if (settings.DefaultResolutionH <= 1080) m_selectedResIdx = 1;
    else if (settings.DefaultResolutionH <= 1440) m_selectedResIdx = 2;
    else m_selectedResIdx = 3;

    // Mapear FPS
    if (settings.DefaultFPS <= 30) m_selectedFpsIdx = 0;
    else if (settings.DefaultFPS <= 60) m_selectedFpsIdx = 1;
    else m_selectedFpsIdx = 2;

    // Mapear Bitrate de Áudio
    if (settings.AudioBitrateKbps <= 64) m_selectedAudioBitrateIdx = 0;
    else if (settings.AudioBitrateKbps <= 128) m_selectedAudioBitrateIdx = 1;
    else m_selectedAudioBitrateIdx = 2;

    ScreenPreviewer::Get().SetModalActive(true);
}

void GoLiveModal::Render() {
    if (!m_isOpen) {
        ScreenPreviewer::Get().SetModalActive(false);
        return;
    }

    ImGui::OpenPopup("Transmitir ao Vivo##GoLiveModal");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(560, 0), ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24, 20));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ThemeDiscord::COLOR_CARD_BG);
    ImGui::PushStyleColor(ImGuiCol_Border, ThemeDiscord::COLOR_BORDER_SUBTLE);

    bool isOpenLocal = m_isOpen;
    if (ImGui::BeginPopupModal("Transmitir ao Vivo##GoLiveModal", &isOpenLocal, 
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)) {
        
        ImDrawList* dl = ImGui::GetWindowDrawList();

        // 1. Cabeçalho com Título e Nome do Amigo
        if (ThemeDiscord::FontHeader) ImGui::PushFont(ThemeDiscord::FontHeader);
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "Transmitir Tela");
        if (ThemeDiscord::FontHeader) ImGui::PopFont();

        if (ThemeDiscord::FontSmall) ImGui::PushFont(ThemeDiscord::FontSmall);
        std::string subTitle = "Compartilhando com " + m_targetFriend.Nickname + " (" + m_targetFriend.RadminIP + ")";
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "%s", subTitle.c_str());
        if (ThemeDiscord::FontSmall) ImGui::PopFont();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 2. Abas de Seleção: [ Telas ] | [ Aplicativos ]
        {
            auto renderTabButton = [&](const char* label, int tabIdx) {
                bool isCur = (m_selectedTab == tabIdx);
                if (isCur) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ThemeDiscord::COLOR_BLURPLE);
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_BLURPLE_HOV);
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(35, 36, 40, 255));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(50, 52, 58, 255));
                }
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
                if (ImGui::Button(label, ImVec2(130, 32))) {
                    m_selectedTab = tabIdx;
                }
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(2);
                ImGui::SameLine();
            };

            renderTabButton("📺  Telas", 0);
            renderTabButton("📱  Aplicativos", 1);
            ImGui::NewLine();
        }

        ImGui::Spacing();

        // 3. Conteúdo da Aba
        if (m_selectedTab == 0) {
            // Aba Telas: Cartões com Thumbnail ao Vivo via SRV
            int monCount = ScreenPreviewer::Get().GetMonitorCount();
            if (monCount <= 0) monCount = 1;

            float cardW = (monCount == 1) ? 360.0f : 240.0f;
            float cardH = (monCount == 1) ? 220.0f : 160.0f;
            float thumbW = cardW - 20.0f;
            float thumbH = thumbW * (9.0f / 16.0f);

            ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "SELECIONE O MONITOR PARA TRANSMITIR:");
            ImGui::Spacing();

            for (int i = 0; i < monCount; ++i) {
                ImGui::PushID(i);

                bool isSelected = (m_selectedMonitorIdx == i);
                ImVec2 pStart = ImGui::GetCursorScreenPos();
                ImVec2 pEnd = ImVec2(pStart.x + cardW, pStart.y + cardH);

                // Fundo do cartão
                ImU32 cardBg = isSelected ? IM_COL32(40, 43, 50, 255) : IM_COL32(30, 31, 35, 255);
                ImU32 borderCol = isSelected ? IM_COL32(88, 101, 242, 255) : IM_COL32(50, 52, 58, 255);
                float borderThick = isSelected ? 2.5f : 1.0f;
                ThemeDiscord::DrawCard(dl, pStart, pEnd, cardBg, 8.0f, borderCol, borderThick);

                // Miniatura ao vivo via ImGui::Image
                ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + 10, ImGui::GetCursorPosY() + 8));
                ID3D11ShaderResourceView* srv = ScreenPreviewer::Get().GetPreviewSRV(i);
                if (srv) {
                    ImGui::Image(reinterpret_cast<ImTextureID>(srv), ImVec2(thumbW, thumbH));
                } else {
                    dl->AddRectFilled(ImVec2(pStart.x + 10, pStart.y + 8), ImVec2(pStart.x + 10 + thumbW, pStart.y + 8 + thumbH), IM_COL32(15, 16, 18, 255), 4.0f);
                }

                // Nome do monitor
                ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + 10, ImGui::GetCursorPosY() + 4));
                std::string monName = ScreenPreviewer::Get().GetMonitorName(i);
                if (ThemeDiscord::FontBold) ImGui::PushFont(ThemeDiscord::FontBold);
                ImGui::TextColored(isSelected ? ThemeDiscord::COLOR_BLURPLE : ThemeDiscord::COLOR_TEXT_PRIMARY, "%s", monName.c_str());
                if (ThemeDiscord::FontBold) ImGui::PopFont();

                // Botão invisível sobre todo o cartão para clique
                ImGui::SetCursorScreenPos(pStart);
                if (ImGui::InvisibleButton("##CardSelectBtn", ImVec2(cardW, cardH))) {
                    m_selectedMonitorIdx = i;
                }

                ImGui::PopID();

                if (i + 1 < monCount && (i % 2 == 0)) {
                    ImGui::SameLine(0, 16);
                } else {
                    ImGui::Spacing();
                }
            }
        } else {
            // Aba Aplicativos (Placeholder polido estilo Discord)
            ImVec2 pStart = ImGui::GetCursorScreenPos();
            float boxW = 512.0f;
            float boxH = 140.0f;
            ImVec2 pEnd = ImVec2(pStart.x + boxW, pStart.y + boxH);
            ThemeDiscord::DrawCard(dl, pStart, pEnd, IM_COL32(30, 31, 35, 255), 8.0f, IM_COL32(50, 52, 58, 255), 1.0f);

            ImGui::SetCursorPos(ImVec2(30, ImGui::GetCursorPosY() + 40));
            if (ThemeDiscord::FontBold) ImGui::PushFont(ThemeDiscord::FontBold);
            ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "Captura Individual de Aplicativos (Em Breve)");
            if (ThemeDiscord::FontBold) ImGui::PopFont();
            ImGui::SetCursorPosX(30);
            ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "Para transmitir jogos ou navegadores em 4K sem lag, use a aba 'Telas'.");
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 40);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 4. Seção: Configuração da Qualidade da Transmissão
        if (ThemeDiscord::FontSmall) ImGui::PushFont(ThemeDiscord::FontSmall);
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "CONFIGURAÇÃO DA TRANSMISSÃO");
        if (ThemeDiscord::FontSmall) ImGui::PopFont();

        ImGui::Spacing();

        // Linha 1: Resolução (Pills)
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "Resolução:");
        ImGui::SameLine(130);
        const char* resLabels[] = { "720p", "1080p", "1440p (2K)", "2160p (4K)" };
        for (int r = 0; r < 4; ++r) {
            bool active = (m_selectedResIdx == r);
            if (active) {
                ImGui::PushStyleColor(ImGuiCol_Button, ThemeDiscord::COLOR_BLURPLE);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_BLURPLE_HOV);
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(35, 36, 40, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(50, 52, 58, 255));
            }
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
            if (ImGui::Button(resLabels[r])) m_selectedResIdx = r;
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(2);
            ImGui::SameLine();
        }
        ImGui::NewLine();

        // Linha 2: Framerate (FPS)
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "Taxa de Quadros:");
        ImGui::SameLine(130);
        const char* fpsLabels[] = { "30 FPS", "60 FPS", "120 FPS" };
        for (int f = 0; f < 3; ++f) {
            bool active = (m_selectedFpsIdx == f);
            if (active) {
                ImGui::PushStyleColor(ImGuiCol_Button, ThemeDiscord::COLOR_BLURPLE);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_BLURPLE_HOV);
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(35, 36, 40, 255));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(50, 52, 58, 255));
            }
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
            if (ImGui::Button(fpsLabels[f])) m_selectedFpsIdx = f;
            ImGui::PopStyleVar();
            ImGui::PopStyleColor(2);
            ImGui::SameLine();
        }
        ImGui::NewLine();

        // Linha 3: Bitrate de Áudio
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "Bitrate de Áudio:");
        ImGui::SameLine(130);
        const char* audioOptions[] = { "64 kbps (Básico)", "128 kbps (Padrão)", "192 kbps (Alta Qualidade)" };
        ImGui::SetNextItemWidth(260);
        ImGui::Combo("##AudioBitrateCombo", &m_selectedAudioBitrateIdx, audioOptions, IM_ARRAYSIZE(audioOptions));

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 5. Rodapé: Botão Cancelar + Botão Blurple "Transmitir ao vivo"
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 320);

        // Cancelar
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(40, 42, 47, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(55, 58, 65, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(65, 68, 77, 255));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        if (ImGui::Button("Cancelar", ImVec2(120, 38))) {
            m_isOpen = false;
            ScreenPreviewer::Get().SetModalActive(false);
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();

        // Transmitir ao vivo
        ImGui::PushStyleColor(ImGuiCol_Button, ThemeDiscord::COLOR_BLURPLE);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_BLURPLE_HOV);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeDiscord::COLOR_BLURPLE_ACT);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        if (ImGui::Button("Transmitir ao vivo", ImVec2(165, 38))) {
            // Gravar configurações no SettingsManager
            auto settings = SettingsManager::Get().GetSettings();
            settings.SelectedMonitorIndex = m_selectedMonitorIdx;

            int resH = 1080;
            if (m_selectedResIdx == 0) resH = 720;
            else if (m_selectedResIdx == 1) resH = 1080;
            else if (m_selectedResIdx == 2) resH = 1440;
            else resH = 2160;
            settings.DefaultResolutionH = resH;

            int fps = 60;
            if (m_selectedFpsIdx == 0) fps = 30;
            else if (m_selectedFpsIdx == 1) fps = 60;
            else fps = 120;
            settings.DefaultFPS = fps;

            int audioBitrate = 128;
            if (m_selectedAudioBitrateIdx == 0) audioBitrate = 64;
            else if (m_selectedAudioBitrateIdx == 1) audioBitrate = 128;
            else audioBitrate = 192;
            settings.AudioBitrateKbps = audioBitrate;

            SettingsManager::Get().UpdateSettings(settings);

            // Disparar confirmação
            if (OnConfirm) {
                OnConfirm(m_targetFriend, resH, fps, m_selectedMonitorIdx);
            }

            m_isOpen = false;
            ScreenPreviewer::Get().SetModalActive(false);
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        ImGui::EndPopup();
    } else {
        m_isOpen = false;
        ScreenPreviewer::Get().SetModalActive(false);
    }

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}
