#include "CaptureEngine.h"
#include "DXGICapture.h"
#include "WGCCapture.h"
#include "../config/SettingsManager.h"
#include <iostream>

CaptureEngine::CaptureEngine() = default;
CaptureEngine::~CaptureEngine() = default;

bool CaptureEngine::Initialize() {
    auto settings = SettingsManager::Get().GetSettings();
    return Initialize(settings.SelectedMonitorIndex, settings.DefaultFPS, settings.UseWGCFallback);
}

bool CaptureEngine::Initialize(int displayIndex, int targetFps) {
    auto settings = SettingsManager::Get().GetSettings();
    if (displayIndex < 0) displayIndex = settings.SelectedMonitorIndex;
    if (targetFps <= 0) targetFps = settings.DefaultFPS;
    return Initialize(displayIndex, targetFps, settings.UseWGCFallback);
}

bool CaptureEngine::Initialize(int displayIndex, int targetFps, bool useWGCFallback) {
    if (displayIndex < 0) {
        auto settings = SettingsManager::Get().GetSettings();
        displayIndex = settings.SelectedMonitorIndex;
    }
    if (targetFps <= 0) {
        auto settings = SettingsManager::Get().GetSettings();
        targetFps = settings.DefaultFPS;
    }

    // Tentar inicializar primariamente a captura DXGI (Zero-Copy VRAM)
    m_source = std::make_unique<DXGICapture>();
    if (m_source->Initialize(displayIndex, targetFps)) {
        std::cout << "[CaptureEngine] DXGI Capture inicializado com sucesso (Monitor " << displayIndex << ", " << targetFps << " FPS)." << std::endl;
        return true;
    }

    if (!useWGCFallback) {
        std::cerr << "[CaptureEngine] DXGI Capture falhou e WGC Fallback esta desativado nas definicoes." << std::endl;
        m_source.reset();
        return false;
    }

    std::cout << "[CaptureEngine] Falha no DXGI Capture. A transitar dinamicamente para WGC..." << std::endl;
    
    // Em caso de falha de DXGI (ex: problemas com Multiplane Overlays), comutar para WGC
    m_source = std::make_unique<WGCCapture>();
    if (m_source->Initialize(displayIndex, targetFps)) {
        std::cout << "[CaptureEngine] WGC Capture inicializado com sucesso (Monitor " << displayIndex << ", " << targetFps << " FPS)." << std::endl;
        return true;
    }

    std::cerr << "[CaptureEngine] ERRO: Falha total na inicializacao de captura (DXGI e WGC falharam)." << std::endl;
    m_source.reset();
    return false;
}

bool CaptureEngine::AcquireFrame(ID3D11Texture2D** ppTexture, uint64_t& timestampUs) {
    if (!m_source) return false;
    return m_source->AcquireFrame(ppTexture, timestampUs);
}

void CaptureEngine::ReleaseFrame() {
    if (m_source) {
        m_source->ReleaseFrame();
    }
}

ID3D11Device* CaptureEngine::GetDevice() {
    return m_source ? m_source->GetDevice() : nullptr;
}
