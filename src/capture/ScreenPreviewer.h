#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <chrono>
#include <vector>
#include <string>
#include <mutex>

using Microsoft::WRL::ComPtr;

struct MonitorThumb {
    int index = 0;
    std::string name;
    RECT rect = { 0, 0, 0, 0 };
    int nativeWidth = 1920;
    int nativeHeight = 1080;
    int refreshRate = 60;
    bool isPrimary = false;
    ComPtr<ID3D11Texture2D> texture;
    ComPtr<ID3D11ShaderResourceView> srv;
    std::vector<uint32_t> pixelBuffer;
};

class ScreenPreviewer {
public:
    static ScreenPreviewer& Get();

    void Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
    void Update();

    // Controle de teste
    void StartTest();
    void StopTest();
    void ToggleTest();
    bool IsTesting() const { return m_isTesting; }

    // Notificação de visibilidade de modais (GoLiveModal ou ScreenTestModal)
    void SetModalActive(bool active) { m_modalActive = active; }
    bool IsModalActive() const { return m_modalActive; }

    // Monitor ativo
    void SetActiveMonitor(int index);
    int GetActiveMonitorIndex() const;

    // Retorna a SRV para o monitor selecionado ou índice específico
    ID3D11ShaderResourceView* GetSRV() const;
    ID3D11ShaderResourceView* GetPreviewSRV(int monitorIndex);

    int GetMonitorCount() const;
    std::string GetMonitorName(int index) const;
    bool IsMonitorPrimary(int index) const;
    int GetMonitorNativeWidth(int index) const;
    int GetMonitorNativeHeight(int index) const;
    int GetMonitorRefreshRate(int index) const;

    int GetWidth() const { return m_previewWidth; }
    int GetHeight() const { return m_previewHeight; }
    float GetCurrentFPS() const { return m_fps; }
    float GetCaptureLatencyMs() const { return m_latencyMs; }
    ID3D11Device* GetDevice() const { return m_device; }

private:
    ScreenPreviewer();
    ~ScreenPreviewer() = default;
    ScreenPreviewer(const ScreenPreviewer&) = delete;
    ScreenPreviewer& operator=(const ScreenPreviewer&) = delete;

    void EnumerateMonitors();
    void CaptureMonitor(MonitorThumb& mon);
    void CreateMonitorResources(MonitorThumb& mon);

    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;

    bool m_isTesting = false;
    bool m_modalActive = false;
    bool m_hasInitialCapture = false;
    int m_activeMonitor = 0;

    // Resolução de captura em definição equilibrada para nitidez e compatibilidade GDI
    const int m_previewWidth = 640;
    const int m_previewHeight = 360;

    std::vector<MonitorThumb> m_monitors;
    mutable std::mutex m_mutex;

    std::chrono::steady_clock::time_point m_lastCaptureTime;
    int m_frameCount = 0;
    float m_fps = 60.0f;
    float m_latencyMs = 0.8f;
    std::chrono::steady_clock::time_point m_lastFpsTime;
};
