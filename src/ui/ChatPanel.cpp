#include "ChatPanel.h"
#include "ThemeDiscord.h"
#include "TextureLoader.h"
#include "../capture/ScreenPreviewer.h"
#include "../config/SettingsManager.h"
#include <imgui.h>
#include <chrono>
#include <iomanip>
#include <sstream>

ChatPanel::ChatPanel() {
    AddLog("ScreenFy 4K inicializado. Pronto para conexao P2P direta.");
    m_goLiveModal.OnConfirm = [this](const UserProfile& target, int resolutionH, int fps, int monitorIdx) {
        (void)monitorIdx;
        if (OnStartCall) {
            OnStartCall(target, resolutionH, fps);
        }
    };
    m_screenTestModal.OnGoLiveRequested = [this]() {
        if (m_currentFriend.has_value()) {
            m_goLiveModal.Open(*m_currentFriend);
        } else {
            AddLog("Selecione um amigo na barra lateral para iniciar a transmissao.");
        }
    };
}

void ChatPanel::AddLog(const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    tm timeinfo;
    localtime_s(&timeinfo, &in_time_t);
    ss << "[" << std::put_time(&timeinfo, "%H:%M:%S") << "] " << message;
    m_logs.push_back(ss.str());
    if (m_logs.size() > 100) {
        m_logs.erase(m_logs.begin());
    }
}

void ChatPanel::Render(const std::optional<UserProfile>& selectedFriend, float width, float height) {
    m_currentFriend = selectedFriend;

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ThemeDiscord::COLOR_WINDOW_BG);
    ImGui::BeginChild("ChatPanelRegion", ImVec2(width, height), false);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    if (!selectedFriend.has_value()) {
        // ==========================================
        // TELA INICIAL: HUB DE IDENTIDADE & FRIEND CODE
        // ==========================================
        ImGui::SetCursorPos(ImVec2(32, 24));
        float hubW = width - 64.0f;
        float hubH = height - 40.0f;
        ImGui::BeginChild("WelcomeHub", ImVec2(hubW, hubH), false);

        // 1. Banner Principal com Botões
        if (ThemeDiscord::FontTitle) ImGui::PushFont(ThemeDiscord::FontTitle);
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "Bem-vindo ao ScreenFy");
        if (ThemeDiscord::FontTitle) ImGui::PopFont();

        ImGui::SameLine(hubW - 350.0f);

        // Botão Tutorial para Amigos
        ImGui::PushStyleColor(ImGuiCol_Button, ThemeDiscord::COLOR_BLURPLE);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_BLURPLE_HOV);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeDiscord::COLOR_BLURPLE_ACT);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        if (ImGui::Button("📖 Como Usar / Tutorial", ImVec2(180, 32))) {
            if (OnOpenTutorial) OnOpenTutorial();
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        ImGui::SameLine();

        // Botão Testar Ecrã Estúdio (Masterizado)
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(35, 165, 90, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(40, 185, 100, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(30, 145, 80, 255));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        if (ImGui::Button("👁 Testar Ecrã", ImVec2(150, 32))) {
            m_screenTestModal.Open();
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        ImGui::Spacing();
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "Transmissao direta ponto-a-ponto em 4K 120 FPS de ultra-baixa latencia sobre Radmin VPN.");

        auto myProf = FriendManager::Instance().GetMyProfile();
        std::string myCode = FriendManager::Instance().GetMyFriendCode();

        ImGui::Spacing();
        ImGui::Spacing();

        // 2. Cartão de Identidade do Utilizador (Gamer Card)
        float cardWidth = hubW;
        float cardHeight = 150.0f;
        ImVec2 cardStart = ImGui::GetCursorScreenPos();
        ImVec2 cardEnd = ImVec2(cardStart.x + cardWidth, cardStart.y + cardHeight);

        ThemeDiscord::DrawCard(dl, cardStart, cardEnd, IM_COL32(43, 45, 49, 255), 10.0f, IM_COL32(56, 58, 64, 255), 1.0f);

        // Avatar
        ID3D11ShaderResourceView* myAvatarSrv = TextureLoader::Get().LoadTextureFromFile(myProf.AvatarPath);
        ImVec2 avatarPos = ImVec2(cardStart.x + 36, cardStart.y + 36);
        ThemeDiscord::DrawAvatar(dl, avatarPos, 24.0f, myProf.Nickname, true, true, myAvatarSrv, static_cast<int>(myProf.Status));

        // Conteúdo Interno do Cartão
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
        ImGui::BeginChild("GamerCardContent", ImVec2(cardWidth, cardHeight), false);

        // Nome
        ImGui::SetCursorPos(ImVec2(72, 14));
        if (ThemeDiscord::FontHeader) ImGui::PushFont(ThemeDiscord::FontHeader);
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "%s", myProf.Nickname.c_str());
        if (ThemeDiscord::FontHeader) ImGui::PopFont();

        // IP Tag
        ImGui::SetCursorPos(ImVec2(72, 38));
        if (ThemeDiscord::FontSmall) ImGui::PushFont(ThemeDiscord::FontSmall);
        std::string ipTag = "IP Radmin VPN: " + myProf.RadminIP + "   •   Porta Sinalização: 49152 (Ativa)";
        ImGui::TextColored(ThemeDiscord::COLOR_GREEN, "%s", ipTag.c_str());
        if (ThemeDiscord::FontSmall) ImGui::PopFont();

        // Linha Divisória
        dl->AddLine(ImVec2(cardStart.x + 20, cardStart.y + 68), ImVec2(cardEnd.x - 20, cardStart.y + 68), IM_COL32(56, 58, 64, 255));

        // Friend Code
        ImGui::SetCursorPos(ImVec2(20, 78));
        if (ThemeDiscord::FontSmall) ImGui::PushFont(ThemeDiscord::FontSmall);
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "O SEU FRIEND CODE DIRETO:");
        if (ThemeDiscord::FontSmall) ImGui::PopFont();

        ImGui::SetCursorPos(ImVec2(20, 98));
        char codeBuf[512];
        strncpy_s(codeBuf, sizeof(codeBuf), myCode.c_str(), _TRUNCATE);
        ImGui::SetNextItemWidth(cardWidth - 210);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(22, 23, 25, 255));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        ImGui::InputText("##myCodeField", codeBuf, sizeof(codeBuf), ImGuiInputTextFlags_ReadOnly);
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ThemeDiscord::COLOR_BLURPLE);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_BLURPLE_HOV);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeDiscord::COLOR_BLURPLE_ACT);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        if (ImGui::Button("Copiar Código", ImVec2(160, 32))) {
            ImGui::SetClipboardText(myCode.c_str());
            m_copiedNotification = true;
            m_copiedTimer = 3.0f;
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        if (m_copiedNotification) {
            ImGui::SameLine();
            ImGui::TextColored(ThemeDiscord::COLOR_GREEN, "✓ Copiado!");
            m_copiedTimer -= ImGui::GetIO().DeltaTime;
            if (m_copiedTimer <= 0.0f) m_copiedNotification = false;
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::Spacing();

        // 3. Três Cartões de Especificações
        float specCardWidth = (hubW - 20.0f) / 3.0f;
        ImVec2 spBasePos = ImGui::GetCursorScreenPos();

        auto drawSpecCard = [&](const char* title, const char* subtitle, const char* desc, int index) {
            ImVec2 spPos = ImVec2(spBasePos.x + index * (specCardWidth + 10.0f), spBasePos.y);
            ImVec2 spEnd = ImVec2(spPos.x + specCardWidth, spPos.y + 90.0f);
            ThemeDiscord::DrawCard(dl, spPos, spEnd, IM_COL32(35, 36, 40, 255), 8.0f, IM_COL32(50, 52, 58, 255), 1.0f);

            if (ThemeDiscord::FontBold) ImGui::PushFont(ThemeDiscord::FontBold);
            dl->AddText(ImVec2(spPos.x + 14, spPos.y + 12), IM_COL32(245, 246, 248, 255), title);
            if (ThemeDiscord::FontBold) ImGui::PopFont();

            if (ThemeDiscord::FontSmall) ImGui::PushFont(ThemeDiscord::FontSmall);
            dl->AddText(ImVec2(spPos.x + 14, spPos.y + 34), IM_COL32(88, 101, 242, 255), subtitle);
            dl->AddText(ImVec2(spPos.x + 14, spPos.y + 56), IM_COL32(148, 155, 164, 255), desc);
            if (ThemeDiscord::FontSmall) ImGui::PopFont();
        };

        drawSpecCard("Resolução 4K UHD", "Captura Zero-Copy DXGI", "Captura direta da VRAM a 120 FPS", 0);
        drawSpecCard("NVENC Low-Latency", "CBR HEVC H.265 85Mbps", "Codificação por GPU sem lag", 1);
        drawSpecCard("Reed-Solomon FEC", "Tolerância a perda UDP", "Recupera pacotes sem reenvio", 2);

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 98.0f);
        ImGui::Spacing();

        // 4. Registo de Atividade
        if (ThemeDiscord::FontSmall) ImGui::PushFont(ThemeDiscord::FontSmall);
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "REGISTO DE EVENTOS & CONEXÕES:");
        if (ThemeDiscord::FontSmall) ImGui::PopFont();

        ImGui::Spacing();
        float logHeight = hubH - ImGui::GetCursorPosY() - 10.0f;
        if (logHeight < 120.0f) logHeight = 120.0f;

        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(22, 23, 25, 255));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 10));
        ImGui::BeginChild("ActivityLogHome", ImVec2(hubW, logHeight), true);
        for (const auto& log : m_logs) {
            ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "%s", log.c_str());
        }
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();

        ImGui::EndChild();
    } else {
        // ==========================================
        // AMIGO SELECIONADO: CABEÇALHO + CONTROLOS COM PREVIEW
        // ==========================================
        const auto& friendProfile = *selectedFriend;

        // 1. Barra de Cabeçalho Superior
        ImVec2 curPos = ImGui::GetCursorScreenPos();
        float headerBarHeight = 70.0f;
        ImVec2 barEnd = ImVec2(curPos.x + width, curPos.y + headerBarHeight);

        dl->AddRectFilled(curPos, barEnd, IM_COL32(26, 27, 30, 255));
        dl->AddLine(ImVec2(curPos.x, barEnd.y), barEnd, IM_COL32(40, 42, 47, 255), 1.0f);

        // Avatar do Amigo
        ID3D11ShaderResourceView* friendAvatarSrv = nullptr;
        if (!friendProfile.AvatarPath.empty()) {
            friendAvatarSrv = TextureLoader::Get().LoadTextureFromFile(friendProfile.AvatarPath);
        }
        ImVec2 friendAvatarPos = ImVec2(curPos.x + 36, curPos.y + 35);
        ThemeDiscord::DrawAvatar(dl, friendAvatarPos, 22.0f, friendProfile.Nickname, friendProfile.IsOnline, true, friendAvatarSrv);

        // Nome do Amigo
        if (ThemeDiscord::FontHeader) ImGui::PushFont(ThemeDiscord::FontHeader);
        dl->AddText(ImVec2(curPos.x + 72, curPos.y + 15), IM_COL32(245, 246, 248, 255), friendProfile.Nickname.c_str());
        if (ThemeDiscord::FontHeader) ImGui::PopFont();

        // Subtítulo com Status e IP
        if (ThemeDiscord::FontSmall) ImGui::PushFont(ThemeDiscord::FontSmall);
        if (friendProfile.IsOnline) {
            std::string st = "ONLINE  |  IP Radmin: " + friendProfile.RadminIP + "  |  Pronto para receber transmissão";
            dl->AddText(ImVec2(curPos.x + 72, curPos.y + 40), IM_COL32(35, 165, 90, 255), st.c_str());
        } else {
            std::string st = "OFFLINE  |  IP Radmin: " + friendProfile.RadminIP + "  |  Aguardando entrada na rede VPN";
            dl->AddText(ImVec2(curPos.x + 72, curPos.y + 40), IM_COL32(148, 155, 164, 255), st.c_str());
        }
        if (ThemeDiscord::FontSmall) ImGui::PopFont();

        // 2. Painel de Transmissão com Pré-visualização ao Vivo
        ImGui::SetCursorPos(ImVec2(24, headerBarHeight + 16));
        float panelWidth = width - 48.0f;

        ImVec2 callBoxStart = ImGui::GetCursorScreenPos();
        float callBoxHeight = 220.0f;
        ImVec2 callBoxEnd = ImVec2(callBoxStart.x + panelWidth, callBoxStart.y + callBoxHeight);
        ThemeDiscord::DrawCard(dl, callBoxStart, callBoxEnd, IM_COL32(43, 45, 49, 255), 10.0f, IM_COL32(56, 58, 64, 255), 1.0f);

        ImGui::BeginChild("CallControlBox", ImVec2(panelWidth, callBoxHeight), false);

        // Coluna Esquerda: Área de Preview de Ecrã
        float previewW = 280.0f;
        float previewH = 158.0f;

        ImVec2 prevStart = ImVec2(callBoxStart.x + 18, callBoxStart.y + 16);
        ImVec2 prevEnd = ImVec2(prevStart.x + previewW, prevStart.y + previewH);

        bool isTesting = ScreenPreviewer::Get().IsTesting();
        if (isTesting) {
            ImGui::SetCursorPos(ImVec2(18, 16));
            ImGui::Image(reinterpret_cast<ImTextureID>(ScreenPreviewer::Get().GetSRV()), ImVec2(previewW, previewH));
            // Badge com FPS
            dl->AddRectFilled(ImVec2(prevStart.x + 8, prevStart.y + 8), ImVec2(prevStart.x + 160, prevStart.y + 32), IM_COL32(0, 0, 0, 190), 4.0f);
            std::string fpsLabel = "🟢 AO VIVO (" + std::to_string(static_cast<int>(ScreenPreviewer::Get().GetCurrentFPS())) + " FPS)";
            dl->AddText(ImVec2(prevStart.x + 14, prevStart.y + 12), IM_COL32(35, 165, 90, 255), fpsLabel.c_str());
        } else {
            // Moldura de Monitor Inativo
            dl->AddRectFilled(prevStart, prevEnd, IM_COL32(23, 24, 27, 255), 8.0f);
            dl->AddRect(prevStart, prevEnd, IM_COL32(50, 52, 58, 255), 8.0f);
            
            if (ThemeDiscord::FontBold) ImGui::PushFont(ThemeDiscord::FontBold);
            dl->AddText(ImVec2(prevStart.x + 36, prevStart.y + 55), IM_COL32(245, 246, 248, 255), "Pré-visualização do Ecrã");
            if (ThemeDiscord::FontBold) ImGui::PopFont();

            if (ThemeDiscord::FontSmall) ImGui::PushFont(ThemeDiscord::FontSmall);
            dl->AddText(ImVec2(prevStart.x + 24, prevStart.y + 80), IM_COL32(148, 155, 164, 255), "Clique abaixo para testar no Estúdio");
            if (ThemeDiscord::FontSmall) ImGui::PopFont();
        }

        // Botão Testar Ecrã (Abre o Estúdio)
        ImGui::SetCursorPos(ImVec2(18, previewH + 22));
        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(35, 36, 40, 255));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_BLURPLE);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeDiscord::COLOR_BLURPLE_ACT);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        if (ImGui::Button("👁  Abrir Estúdio de Teste", ImVec2(previewW, 32))) {
            m_screenTestModal.Open();
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        // Linha Divisória Vertical
        dl->AddLine(ImVec2(callBoxStart.x + previewW + 36, callBoxStart.y + 16), ImVec2(callBoxStart.x + previewW + 36, callBoxEnd.y - 16), IM_COL32(56, 58, 64, 255));

        // Coluna Direita: Informações e Botão "Compartilhar Tela"
        float rightColX = previewW + 54.0f;
        ImGui::SetCursorPos(ImVec2(rightColX, 16));

        if (ThemeDiscord::FontBold) ImGui::PushFont(ThemeDiscord::FontBold);
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "Transmissão ScreenFy 4K");
        if (ThemeDiscord::FontBold) ImGui::PopFont();

        ImGui::SetCursorPos(ImVec2(rightColX, 42));
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "Transmissao direta ponto-a-ponto de ultra-baixa latencia.");
        ImGui::SetCursorPos(ImVec2(rightColX, 62));
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "Aceleracao por GPU NVENC e som de alta fidelidade.");

        // Informações da transmissão atual
        auto currentSettings = SettingsManager::Get().GetSettings();
        std::string streamInfo = "Monitor " + std::to_string(currentSettings.SelectedMonitorIndex + 1) + 
                                 "  •  " + std::to_string(currentSettings.DefaultResolutionH) + "p  •  " + 
                                 std::to_string(currentSettings.DefaultFPS) + " FPS";
        ImGui::SetCursorPos(ImVec2(rightColX, 88));
        ImGui::TextColored(ThemeDiscord::COLOR_BLURPLE, "%s", streamInfo.c_str());

        // Botão Principal "Compartilhar Tela"
        ImGui::SetCursorPos(ImVec2(rightColX, 122));
        bool canCall = friendProfile.IsOnline;
        if (!canCall) {
            ImGui::BeginDisabled();
        }

        ImGui::PushStyleColor(ImGuiCol_Button, ThemeDiscord::COLOR_GREEN);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_GREEN_HOV);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeDiscord::COLOR_GREEN_ACT);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

        if (ImGui::Button(" 📺   Compartilhar Tela", ImVec2(panelWidth - rightColX - 20, 46))) {
            m_goLiveModal.Open(friendProfile);
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        if (!canCall) {
            ImGui::EndDisabled();
            ImGui::SetCursorPos(ImVec2(rightColX, 178));
            ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "Aguardando que o amigo esteja Online no Radmin VPN...");
        }

        ImGui::EndChild();

        // 3. Registo de Atividade do Chat
        ImGui::SetCursorPos(ImVec2(24, headerBarHeight + callBoxHeight + 28));
        if (ThemeDiscord::FontSmall) ImGui::PushFont(ThemeDiscord::FontSmall);
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "REGISTO DE ATIVIDADE COM ESTE CONTACTO:");
        if (ThemeDiscord::FontSmall) ImGui::PopFont();

        ImGui::SetCursorPos(ImVec2(24, headerBarHeight + callBoxHeight + 50));
        float chatLogHeight = height - (headerBarHeight + callBoxHeight + 68);
        if (chatLogHeight < 100) chatLogHeight = 100;

        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(22, 23, 25, 255));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 10));
        ImGui::BeginChild("ActivityLogFriend", ImVec2(panelWidth, chatLogHeight), true);
        for (const auto& log : m_logs) {
            ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "%s", log.c_str());
        }
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    // Renderizar Modais caso estejam ativos
    m_goLiveModal.Render();
    m_screenTestModal.Render();
}
