#include "DXGICapture.h"
#include <iostream>
#include <stdexcept>
#include <windows.h>

DXGICapture::DXGICapture() : m_targetFps(120) {}
DXGICapture::~DXGICapture() = default;

bool DXGICapture::Initialize(int displayIndex, int targetFps) {
    m_targetFps = targetFps;

    // Criar o ID3D11Device com a flag D3D11_CREATE_DEVICE_VIDEO_SUPPORT (Lote 1)
    UINT createDeviceFlags = D3D11_CREATE_DEVICE_VIDEO_SUPPORT;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
    D3D_FEATURE_LEVEL featureLevel;

    HRESULT hr = D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
        featureLevels, 2, D3D11_SDK_VERSION, &m_device, &featureLevel, &m_context
    );

    if (FAILED(hr)) {
        std::cerr << "D3D11CreateDevice falhou: " << std::hex << hr << std::endl;
        return false;
    }

    ComPtr<IDXGIDevice> dxgiDevice;
    hr = m_device.As(&dxgiDevice);
    if (FAILED(hr)) return false;

    ComPtr<IDXGIAdapter> dxgiAdapter;
    hr = dxgiDevice->GetAdapter(&dxgiAdapter);
    if (FAILED(hr)) return false;

    ComPtr<IDXGIOutput> dxgiOutput;
    hr = dxgiAdapter->EnumOutputs(displayIndex, &dxgiOutput);
    if (FAILED(hr)) {
        std::cerr << "EnumOutputs falhou para o ecrã " << displayIndex << std::endl;
        return false;
    }

    ComPtr<IDXGIOutput1> dxgiOutput1;
    hr = dxgiOutput.As(&dxgiOutput1);
    if (FAILED(hr)) return false;

    hr = dxgiOutput1->DuplicateOutput(m_device.Get(), &m_deskDupl);
    if (FAILED(hr)) {
        std::cerr << "DuplicateOutput falhou (talvez conflito MPO?): " << std::hex << hr << std::endl;
        return false;
    }

    return true;
}

bool DXGICapture::AcquireFrame(ID3D11Texture2D** ppTexture, uint64_t& timestampUs) {
    if (!m_deskDupl) return false;

    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    ComPtr<IDXGIResource> desktopResource;
    
    // AcquireNextFrame com tempo 0 para tempo de resposta nulo
    HRESULT hr = m_deskDupl->AcquireNextFrame(0, &frameInfo, &desktopResource);

    if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
        // Timeout: reaproveitar textura anterior (sem cópia de memória RAM)
        if (m_lastFrame) {
            *ppTexture = m_lastFrame.Get();
            (*ppTexture)->AddRef();
            
            LARGE_INTEGER qpc;
            QueryPerformanceCounter(&qpc);
            timestampUs = qpc.QuadPart;
            return true;
        }
        return false;
    }

    if (FAILED(hr)) {
        return false;
    }

    ComPtr<ID3D11Texture2D> tex2D;
    hr = desktopResource.As(&tex2D);
    if (FAILED(hr)) {
        m_deskDupl->ReleaseFrame();
        return false;
    }

    m_lastFrame = tex2D;
    
    *ppTexture = m_lastFrame.Get();
    (*ppTexture)->AddRef();
    
    timestampUs = frameInfo.LastPresentTime.QuadPart;
    if (timestampUs == 0) {
        LARGE_INTEGER qpc;
        QueryPerformanceCounter(&qpc);
        timestampUs = qpc.QuadPart;
    }

    return true;
}

void DXGICapture::ReleaseFrame() {
    if (m_deskDupl) {
        m_deskDupl->ReleaseFrame();
    }
}
