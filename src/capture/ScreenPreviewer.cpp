#include "ScreenPreviewer.h"
#include "../config/SettingsManager.h"
#include <iostream>

ScreenPreviewer &ScreenPreviewer::Get() {
  static ScreenPreviewer instance;
  return instance;
}

ScreenPreviewer::ScreenPreviewer() {
  m_lastCaptureTime = std::chrono::steady_clock::now();
  m_lastFpsTime = std::chrono::steady_clock::now();
}

namespace {
struct EnumData {
  std::vector<MonitorThumb> *list;
};

BOOL CALLBACK MonitorCallback(HMONITOR hMonitor, HDC, LPRECT, LPARAM dwData) {
  auto *data = reinterpret_cast<EnumData *>(dwData);
  MONITORINFOEXA mi;
  ZeroMemory(&mi, sizeof(mi));
  mi.cbSize = sizeof(MONITORINFOEXA);
  if (GetMonitorInfoA(hMonitor, &mi)) {
    MonitorThumb thumb;
    thumb.index = static_cast<int>(data->list->size());
    thumb.rect = mi.rcMonitor;
    thumb.isPrimary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
    int w = mi.rcMonitor.right - mi.rcMonitor.left;
    int h = mi.rcMonitor.bottom - mi.rcMonitor.top;
    thumb.nativeWidth = w;
    thumb.nativeHeight = h;

    DEVMODEA dm;
    ZeroMemory(&dm, sizeof(dm));
    dm.dmSize = sizeof(DEVMODEA);
    if (EnumDisplaySettingsA(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm)) {
      thumb.refreshRate =
          dm.dmDisplayFrequency > 0 ? dm.dmDisplayFrequency : 60;
    } else {
      thumb.refreshRate = 60;
    }

    thumb.name = "Tela " + std::to_string(thumb.index + 1) + " (" +
                 std::to_string(w) + "x" + std::to_string(h) + " @" +
                 std::to_string(thumb.refreshRate) + "Hz)";
    if (thumb.isPrimary) {
      thumb.name += " [Principal]";
    }
    data->list->push_back(std::move(thumb));
  }
  return TRUE;
}
} // namespace

void ScreenPreviewer::EnumerateMonitors() {
  std::lock_guard<std::mutex> lock(m_mutex);
  m_monitors.clear();
  EnumData data{&m_monitors};
  EnumDisplayMonitors(nullptr, nullptr, MonitorCallback,
                      reinterpret_cast<LPARAM>(&data));

  if (m_monitors.empty()) {
    MonitorThumb fallback;
    fallback.index = 0;
    fallback.name = "Tela 1 (Principal)";
    fallback.nativeWidth = GetSystemMetrics(SM_CXSCREEN);
    fallback.nativeHeight = GetSystemMetrics(SM_CYSCREEN);
    fallback.refreshRate = 60;
    fallback.rect = {0, 0, fallback.nativeWidth, fallback.nativeHeight};
    fallback.isPrimary = true;
    m_monitors.push_back(std::move(fallback));
  }

  for (auto &mon : m_monitors) {
    CreateMonitorResources(mon);
  }
}

void ScreenPreviewer::CreateMonitorResources(MonitorThumb &mon) {
  if (!m_device)
    return;

  mon.pixelBuffer.assign(m_previewWidth * m_previewHeight, 0xFF1E1F22);

  D3D11_TEXTURE2D_DESC desc = {};
  desc.Width = m_previewWidth;
  desc.Height = m_previewHeight;
  desc.MipLevels = 1;
  desc.ArraySize = 1;
  desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  desc.SampleDesc.Count = 1;
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  desc.CPUAccessFlags = 0;

  D3D11_SUBRESOURCE_DATA initData = {};
  initData.pSysMem = mon.pixelBuffer.data();
  initData.SysMemPitch = m_previewWidth * sizeof(uint32_t);

  HRESULT hr = m_device->CreateTexture2D(&desc, &initData,
                                         mon.texture.ReleaseAndGetAddressOf());
  if (FAILED(hr)) {
    std::cerr << "[ScreenPreviewer] Falha ao criar textura D3D11 para monitor "
              << mon.index << std::endl;
    return;
  }

  D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
  srvDesc.Format = desc.Format;
  srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  srvDesc.Texture2D.MipLevels = 1;

  hr = m_device->CreateShaderResourceView(mon.texture.Get(), &srvDesc,
                                          mon.srv.ReleaseAndGetAddressOf());
  if (FAILED(hr)) {
    std::cerr << "[ScreenPreviewer] Falha ao criar SRV para monitor "
              << mon.index << std::endl;
  }
}

void ScreenPreviewer::Initialize(ID3D11Device *device,
                                 ID3D11DeviceContext *context) {
  m_device = device;
  m_context = context;
  EnumerateMonitors();
  m_activeMonitor = SettingsManager::Get().GetSettings().SelectedMonitorIndex;
}

void ScreenPreviewer::StartTest() {
  m_isTesting = true;
  m_frameCount = 0;
  m_fps = 60.0f;
  m_lastFpsTime = std::chrono::steady_clock::now();
}

void ScreenPreviewer::StopTest() { m_isTesting = false; }

void ScreenPreviewer::ToggleTest() {
  if (m_isTesting) {
    StopTest();
  } else {
    StartTest();
  }
}

void ScreenPreviewer::SetActiveMonitor(int index) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (index >= 0 && index < static_cast<int>(m_monitors.size())) {
    m_activeMonitor = index;
  }
}

int ScreenPreviewer::GetActiveMonitorIndex() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return m_activeMonitor;
}

void ScreenPreviewer::CaptureMonitor(MonitorThumb &mon) {
  if (!m_context || !mon.texture)
    return;

  auto tStart = std::chrono::high_resolution_clock::now();

  int srcX = mon.rect.left;
  int srcY = mon.rect.top;
  int srcW = mon.rect.right - mon.rect.left;
  int srcH = mon.rect.bottom - mon.rect.top;

  if (srcW <= 0 || srcH <= 0) {
    srcX = 0;
    srcY = 0;
    srcW = GetSystemMetrics(SM_CXSCREEN);
    srcH = GetSystemMetrics(SM_CYSCREEN);
  }

  HDC hdcScreen = GetDC(nullptr);
  HDC hdcMem = CreateCompatibleDC(hdcScreen);
  HBITMAP hBitmap =
      CreateCompatibleBitmap(hdcScreen, m_previewWidth, m_previewHeight);
  HGDIOBJ oldBmp = SelectObject(hdcMem, hBitmap);

  SetStretchBltMode(hdcMem, COLORONCOLOR);
  SetBrushOrgEx(hdcMem, 0, 0, nullptr);
  StretchBlt(hdcMem, 0, 0, m_previewWidth, m_previewHeight, hdcScreen, srcX,
             srcY, srcW, srcH, SRCCOPY | CAPTUREBLT);

  BITMAPINFO bi = {};
  bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bi.bmiHeader.biWidth = m_previewWidth;
  bi.bmiHeader.biHeight = -m_previewHeight; // Top-down
  bi.bmiHeader.biPlanes = 1;
  bi.bmiHeader.biBitCount = 32;
  bi.bmiHeader.biCompression = BI_RGB;

  GetDIBits(hdcMem, hBitmap, 0, m_previewHeight, mon.pixelBuffer.data(), &bi,
            DIB_RGB_COLORS);

  // CRÍTICO: GDI preenche o canal alfa com 0x00. No DirectX/ImGui com alpha blending,
  // isso torna a imagem 100% transparente (preta). Forçamos alpha opaco (0xFF).
  for (auto &px : mon.pixelBuffer) {
    px |= 0xFF000000;
  }

  SelectObject(hdcMem, oldBmp);
  DeleteObject(hBitmap);
  DeleteDC(hdcMem);
  ReleaseDC(nullptr, hdcScreen);

  m_context->UpdateSubresource(mon.texture.Get(), 0, nullptr,
                               mon.pixelBuffer.data(),
                               m_previewWidth * sizeof(uint32_t), 0);

  auto tEnd = std::chrono::high_resolution_clock::now();
  m_latencyMs = std::chrono::duration<float, std::milli>(tEnd - tStart).count();
}

void ScreenPreviewer::Update() {
  if (!m_device || !m_context)
    return;

  // Se ainda não capturamos inicialmente, captura uma vez para que os
  // thumbnails existam
  if (!m_hasInitialCapture) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto &mon : m_monitors) {
      CaptureMonitor(mon);
    }
    m_hasInitialCapture = true;
    return;
  }

  if (!m_isTesting && !m_modalActive)
    return;

  auto now = std::chrono::steady_clock::now();
  auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                       now - m_lastCaptureTime)
                       .count();
  if (elapsedMs < 16) {
    return;
  }
  m_lastCaptureTime = now;

  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_modalActive) {
    // No modal, atualiza os monitores
    for (auto &mon : m_monitors) {
      CaptureMonitor(mon);
    }
  } else if (m_isTesting) {
    // No teste, atualiza o monitor ativo
    if (m_activeMonitor >= 0 &&
        m_activeMonitor < static_cast<int>(m_monitors.size())) {
      CaptureMonitor(m_monitors[m_activeMonitor]);
    } else if (!m_monitors.empty()) {
      CaptureMonitor(m_monitors[0]);
    }
  }

  // Cálculo de FPS
  m_frameCount++;
  auto fpsElapsedMs =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastFpsTime)
          .count();
  if (fpsElapsedMs >= 500) {
    m_fps = (m_frameCount * 1000.0f) / fpsElapsedMs;
    m_frameCount = 0;
    m_lastFpsTime = now;
  }
}

ID3D11ShaderResourceView *ScreenPreviewer::GetSRV() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_monitors.empty())
    return nullptr;
  if (m_activeMonitor >= 0 &&
      m_activeMonitor < static_cast<int>(m_monitors.size())) {
    return m_monitors[m_activeMonitor].srv.Get();
  }
  return m_monitors[0].srv.Get();
}

ID3D11ShaderResourceView *ScreenPreviewer::GetPreviewSRV(int monitorIndex) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (monitorIndex >= 0 && monitorIndex < static_cast<int>(m_monitors.size())) {
    return m_monitors[monitorIndex].srv.Get();
  }
  if (!m_monitors.empty()) {
    return m_monitors[0].srv.Get();
  }
  return nullptr;
}

int ScreenPreviewer::GetMonitorCount() const {
  std::lock_guard<std::mutex> lock(m_mutex);
  return static_cast<int>(m_monitors.size());
}

std::string ScreenPreviewer::GetMonitorName(int index) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (index >= 0 && index < static_cast<int>(m_monitors.size())) {
    return m_monitors[index].name;
  }
  return "Monitor Desconhecido";
}

bool ScreenPreviewer::IsMonitorPrimary(int index) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (index >= 0 && index < static_cast<int>(m_monitors.size())) {
    return m_monitors[index].isPrimary;
  }
  return false;
}

int ScreenPreviewer::GetMonitorNativeWidth(int index) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (index >= 0 && index < static_cast<int>(m_monitors.size())) {
    return m_monitors[index].nativeWidth;
  }
  return 1920;
}

int ScreenPreviewer::GetMonitorNativeHeight(int index) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (index >= 0 && index < static_cast<int>(m_monitors.size())) {
    return m_monitors[index].nativeHeight;
  }
  return 1080;
}

int ScreenPreviewer::GetMonitorRefreshRate(int index) const {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (index >= 0 && index < static_cast<int>(m_monitors.size())) {
    return m_monitors[index].refreshRate;
  }
  return 60;
}
