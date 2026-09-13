#include "StreamHealthMonitor.h"
#include "../utils/Logger.h"

StreamHealthMonitor& StreamHealthMonitor::Instance() {
    static StreamHealthMonitor instance;
    return instance;
}

StreamHealthMonitor::StreamHealthMonitor() = default;
StreamHealthMonitor::~StreamHealthMonitor() = default;

void StreamHealthMonitor::Reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_status = StreamHealthStatus();
    m_captureBlackFrames = 0;
    m_encoderEmptyFrames = 0;
    if (OnStatusChanged) OnStatusChanged(m_status);
}

StreamHealthStatus StreamHealthMonitor::GetStatus() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_status;
}

void StreamHealthMonitor::SetStatus(HealthStep step) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_status.isFailed) return; // Não atualizar se falhou
    
    m_status.step = step;
    if (OnStatusChanged) OnStatusChanged(m_status);
}

void StreamHealthMonitor::Fail(const std::string& reason) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_status.isFailed = true;
    m_status.errorMessage = reason;
    m_status.step = HealthStep::Failed;
    LOG_ERROR("HEALTH", "Stream Fail: " + reason);
    if (OnStatusChanged) OnStatusChanged(m_status);
}

void StreamHealthMonitor::UpdateTelemetry(float fps, float pingMs, float packetLoss, float bitrateMbps) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_status.fps = fps;
    m_status.pingMs = pingMs;
    m_status.packetLossPercent = packetLoss;
    m_status.bitrateMbps = bitrateMbps;
    // Opcionalmente invocar callback se quiser atualizações parciais
}

bool StreamHealthMonitor::HookCaptureFrame(ID3D11Texture2D* pTexture, ID3D11DeviceContext* pContext) {
    if (!pTexture || !pContext) return false;

    // Etapa 1: Validar heurística de Ecrã Preto (DRM / Bloqueio UAC)
    // Para performance, só validamos os primeiros frames
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_status.step >= HealthStep::EncoderNVENC || m_status.isFailed) return true;

    // (Simplificação da heurística) Assume-se que o driver D3D11 já mapeou 
    // ou se falhar o Map repetidamente, é um ecrã preto.
    // Vamos simular aqui: Na vida real usaria Staging Texture para ler.
    // Por enquanto, consideramos validado se a textura existe.
    // Se fosse um ecrã preto de DRM, o DXGI DDA retornaria falha na aquisição ou uma textura toda a zeros.
    
    // Aqui podiamos fazer: pContext->Map()
    // Neste protótipo, assumimos sucesso se a captura foi bem sucedida
    m_status.step = HealthStep::EncoderNVENC;
    if (OnStatusChanged) OnStatusChanged(m_status);
    
    return true;
}

void StreamHealthMonitor::HookEncoderOutput(size_t totalBytes, int frameCount) {
    (void)frameCount;
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_status.step >= HealthStep::NetworkRoute || m_status.isFailed) return;
    
    if (totalBytes > 0) {
        m_status.step = HealthStep::NetworkRoute;
        if (OnStatusChanged) OnStatusChanged(m_status);
    } else {
        m_encoderEmptyFrames++;
        if (m_encoderEmptyFrames > 30) {
            // Falhou
        }
    }
}

void StreamHealthMonitor::HookUdpPing(bool success) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_status.step >= HealthStep::PeerAck || m_status.isFailed) return;
    
    if (success) {
        m_status.step = HealthStep::PeerAck;
        if (OnStatusChanged) OnStatusChanged(m_status);
    }
}

void StreamHealthMonitor::HookPeerAck(bool success) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_status.step >= HealthStep::Connected || m_status.isFailed) return;
    
    if (success) {
        m_status.step = HealthStep::Connected;
        if (OnStatusChanged) OnStatusChanged(m_status);
    }
}
