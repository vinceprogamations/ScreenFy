#include "ConnectionOverlay.h"
#include "../engine/StreamHealthMonitor.h"
#include "ThemeDiscord.h"
#include <imgui.h>

ConnectionOverlay::ConnectionOverlay() = default;

void ConnectionOverlay::StartConnection(const std::string& peerName, const std::string& peerIp, bool isHost) {
    m_isActive = true;
    m_isHost = isHost;
    m_peerName = peerName;
    m_peerIp = peerIp;
}

void ConnectionOverlay::CancelConnection() {
    m_isActive = false;
    if (OnConnectionCancelled) OnConnectionCancelled();
}

void ConnectionOverlay::Render(float screenWidth, float screenHeight) {
    if (!m_isActive) return;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(screenWidth, screenHeight));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(14, 15, 17, 245));
    
    ImGui::Begin("ConnectionOverlayWindow", nullptr, 
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

    ImVec2 center = ImVec2(screenWidth * 0.5f, screenHeight * 0.5f);
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Título
    std::string title = m_isHost ? "A Iniciar Transmissao..." : "A Conectar ao Host...";
    ImFont* fontTitle = ThemeDiscord::FontHeader ? ThemeDiscord::FontHeader : ThemeDiscord::FontRegular;
    if (fontTitle) {
        ImVec2 titleSize = fontTitle->CalcTextSizeA(24.0f, FLT_MAX, 0.0f, title.c_str());
        dl->AddText(fontTitle, 24.0f, ImVec2(center.x - titleSize.x * 0.5f, center.y - 120), IM_COL32(245, 246, 248, 255), title.c_str());
    }

    // Status Checklist
    StreamHealthStatus status = StreamHealthMonitor::Instance().GetStatus();

    auto renderChecklist = [&](const char* label, HealthStep requiredStep, float offsetY) {
        bool isDone = (status.step >= requiredStep);
        bool isFailed = status.isFailed && (status.step == HealthStep::Failed);
        
        // Simples semáforo
        const char* icon = "⏳";
        ImU32 color = ImGui::ColorConvertFloat4ToU32(ThemeDiscord::COLOR_TEXT_MUTED);
        
        if (isDone) {
            icon = "✅";
            color = ImGui::ColorConvertFloat4ToU32(ThemeDiscord::COLOR_GREEN);
        } else if (isFailed) {
            icon = "❌";
            color = ImGui::ColorConvertFloat4ToU32(ThemeDiscord::COLOR_RED);
        }

        ImFont* font = ThemeDiscord::FontRegular;
        if (font) {
            std::string text = std::string(icon) + " " + label;
            ImVec2 textSize = font->CalcTextSizeA(18.0f, FLT_MAX, 0.0f, text.c_str());
            dl->AddText(font, 18.0f, ImVec2(center.x - textSize.x * 0.5f, center.y - 50 + offsetY), color, text.c_str());
        }
    };

    if (m_isHost) {
        renderChecklist("Inicializando Captura da GPU", HealthStep::EncoderNVENC, 0);
        renderChecklist("Preparando Codificador", HealthStep::NetworkRoute, 30);
        renderChecklist("Estabelecendo Rota Segura", HealthStep::PeerAck, 60);
        renderChecklist("Aguardando Sinal do Destinatario", HealthStep::Connected, 90);
    } else {
        renderChecklist("Preparando Rede UDP", HealthStep::NetworkRoute, 0);
        renderChecklist("Estabelecendo Rota Segura", HealthStep::PeerAck, 30);
        renderChecklist("Aguardando Primeiro Frame (Host)", HealthStep::Connected, 60);
    }

    if (status.step == HealthStep::Connected) {
        m_isActive = false;
        if (OnConnectionEstablished) OnConnectionEstablished();
    }

    if (status.isFailed) {
        ImFont* font = ThemeDiscord::FontRegular;
        if (font) {
            std::string error = "Erro: " + status.errorMessage;
            ImVec2 errorSize = font->CalcTextSizeA(18.0f, FLT_MAX, 0.0f, error.c_str());
            dl->AddText(font, 18.0f, ImVec2(center.x - errorSize.x * 0.5f, center.y + 140), ImGui::ColorConvertFloat4ToU32(ThemeDiscord::COLOR_RED), error.c_str());
        }

        ImGui::SetCursorPos(ImVec2(center.x - 60, center.y + 180));
        ImGui::PushStyleColor(ImGuiCol_Button, ThemeDiscord::COLOR_RED);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_RED_HOV);
        if (ImGui::Button("Cancelar", ImVec2(120, 40))) {
            CancelConnection();
        }
        ImGui::PopStyleColor(2);
    }

    ImGui::End();
    ImGui::PopStyleColor();
}
