#include "OverlayCall.h"
#include "ThemeDiscord.h"
#include <imgui.h>
#include <iostream>

OverlayCall::OverlayCall() = default;

void OverlayCall::SetInCall(bool inCall, bool isHost, const std::string& peerName, const std::string& peerIp) {
    m_isInCall = inCall;
    m_isHost = isHost;
    m_peerName = peerName;
    m_peerIp = peerIp;
    m_hoverTimer = 3.0f;
}

void OverlayCall::UpdateTelemetry(float fps, float latencyMs, float bitrateMbps, float packetLoss) {
    m_fps = fps;
    m_latencyMs = latencyMs;
    m_bitrateMbps = bitrateMbps;
    m_packetLoss = packetLoss;
}

void OverlayCall::Render(float screenWidth, float screenHeight, ID3D11ShaderResourceView* pVideoSRV) {
    if (!m_isInCall) return;

    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* dl = ImGui::GetForegroundDrawList();

    // 1. Renderizar Janela de Ecrã Inteiro com o Vídeo
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(screenWidth, screenHeight));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(14, 15, 17, 255));
    ImGui::Begin("FullscreenVideoViewport", nullptr, 
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);

    if (pVideoSRV) {
        ImGui::Image(reinterpret_cast<ImTextureID>(pVideoSRV), ImVec2(screenWidth, screenHeight));
    } else {
        // Placeholder estilizado quando ainda aguarda o primeiro frame
        ImVec2 center = ImVec2(screenWidth * 0.5f, screenHeight * 0.42f);

        // Avatar do Parceiro no centro
        ThemeDiscord::DrawAvatar(dl, center, 42.0f, m_peerName, true, true);

        std::string title = m_isHost ? "TRANSMITINDO O SEU ECRA EM ULTRA-BAIXA LATENCIA" : "A RECEBER TRANSMISSAO DE ECRA";
        ImFont* fontTitle = ThemeDiscord::FontHeader ? ThemeDiscord::FontHeader : ThemeDiscord::FontRegular;
        if (fontTitle) {
            ImVec2 titleSize = fontTitle->CalcTextSizeA(20.0f, FLT_MAX, 0.0f, title.c_str());
            dl->AddText(fontTitle, 20.0f, ImVec2(center.x - titleSize.x * 0.5f, center.y + 60), IM_COL32(245, 246, 248, 255), title.c_str());
        }

        std::string peerInfo = "Conexao P2P com: " + m_peerName + " (" + m_peerIp + ")";
        ImFont* fontSub = ThemeDiscord::FontRegular;
        if (fontSub) {
            ImVec2 peerSize = fontSub->CalcTextSizeA(15.0f, FLT_MAX, 0.0f, peerInfo.c_str());
            dl->AddText(fontSub, 15.0f, ImVec2(center.x - peerSize.x * 0.5f, center.y + 92), IM_COL32(148, 155, 164, 255), peerInfo.c_str());

            std::string proto = "Pipeline: NVENC HEVC / WASAPI Opus / UDP RTP FEC (4K 120 FPS)";
            ImVec2 protoSize = fontSub->CalcTextSizeA(14.0f, FLT_MAX, 0.0f, proto.c_str());
            dl->AddText(fontSub, 14.0f, ImVec2(center.x - protoSize.x * 0.5f, center.y + 116), IM_COL32(88, 101, 242, 255), proto.c_str());
        }
    }

    ImGui::End();
    ImGui::PopStyleColor();

    // 2. HUD de Telemetria no Canto Superior Direito
    float hudWidth = 260.0f;
    float hudHeight = 116.0f;
    ImGui::SetNextWindowPos(ImVec2(screenWidth - hudWidth - 24, 24));
    ImGui::SetNextWindowSize(ImVec2(hudWidth, hudHeight));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(20, 21, 24, 230));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(45, 47, 54, 200));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 12));

    ImGui::Begin("TelemetryHUD", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    
    // Status do stream (Semáforo Dinâmico)
    const char* statusIcon = "●";
    ImU32 statusColor = ImGui::ColorConvertFloat4ToU32(ThemeDiscord::COLOR_GREEN);
    const char* statusText = "EM DIRETO (P2P)";
    
    if (m_packetLoss >= 15.0f || m_latencyMs > 150.0f) {
        statusColor = ImGui::ColorConvertFloat4ToU32(ThemeDiscord::COLOR_RED);
        statusText = "CONEXAO CRITICA";
    } else if (m_packetLoss >= 5.0f || m_latencyMs > 60.0f) {
        statusColor = IM_COL32(250, 166, 26, 255); // Amarelo Discord
        statusText = "REDE INSTAVEL (FEC ATIVO)";
    }

    ImGui::PushStyleColor(ImGuiCol_Text, statusColor);
    ImGui::Text("%s  %s", statusIcon, statusText);
    ImGui::PopStyleColor();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Grid 2x2 de métricas
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "FPS:");
    ImGui::SameLine(46);
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "%.1f", m_fps);

    ImGui::SameLine(135);
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "Ping:");
    ImGui::SameLine(175);
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "%.1f ms", m_latencyMs);

    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "Bitrate:");
    ImGui::SameLine(58);
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "%.1f M", m_bitrateMbps);

    ImGui::SameLine(135);
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "Loss:");
    ImGui::SameLine(175);
    ImGui::TextColored(m_packetLoss > 2.0f ? ThemeDiscord::COLOR_RED : ThemeDiscord::COLOR_TEXT_PRIMARY, "%.1f%%", m_packetLoss);

    ImGui::End();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);

    // 3. Barra Flutuante Inferior (Dock estilo Discord com Rounding 30px)
    float barWidth = 300.0f;
    float barHeight = 56.0f;
    float barX = (screenWidth - barWidth) * 0.5f;
    float barY = screenHeight - barHeight - 28.0f;

    bool isMouseNear = (io.MousePos.y > (screenHeight - 150.0f));
    if (isMouseNear) {
        m_hoverTimer = 2.5f;
    } else {
        m_hoverTimer -= io.DeltaTime;
    }

    if (m_hoverTimer > 0.0f) {
        ImGui::SetNextWindowPos(ImVec2(barX, barY));
        ImGui::SetNextWindowSize(ImVec2(barWidth, barHeight));

        ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(23, 24, 27, 240));
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(50, 52, 58, 220));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 28.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18, 8));

        ImGui::Begin("CallControlToolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

        ImGui::SetCursorPos(ImVec2(22, 10));

        // Botão Mutar Microfone
        if (m_isMuted) {
            ImGui::PushStyleColor(ImGuiCol_Button, ThemeDiscord::COLOR_RED);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_RED_HOV);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeDiscord::COLOR_RED_ACT);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(43, 45, 49, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(60, 63, 69, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(70, 74, 82, 255));
        }

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 18.0f);
        if (ImGui::Button(m_isMuted ? "🎙 Mutado" : "🎙 Microfone", ImVec2(120, 36))) {
            m_isMuted = !m_isMuted;
            if (OnToggleMute) OnToggleMute(m_isMuted);
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        ImGui::SameLine(158);

        // Botão Desligar
        ImGui::PushStyleColor(ImGuiCol_Button, ThemeDiscord::COLOR_RED);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_RED_HOV);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeDiscord::COLOR_RED_ACT);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 18.0f);
        if (ImGui::Button("📞 Desligar", ImVec2(120, 36))) {
            m_isInCall = false;
            if (OnEndCall) OnEndCall();
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);

        ImGui::End();
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(2);
    }
}
