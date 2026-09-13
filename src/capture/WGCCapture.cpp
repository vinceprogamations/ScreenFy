#include "WGCCapture.h"
#include <iostream>
#include <windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.directx.direct3d11.interop.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <atomic>
#include <mutex>

#pragma comment(lib, "windowsapp")

using Microsoft::WRL::ComPtr;
namespace winrt_capture = winrt::Windows::Graphics::Capture;
namespace winrt_directx = winrt::Windows::Graphics::DirectX;
namespace winrt_d3d11 = winrt::Windows::Graphics::DirectX::Direct3D11;

class WGCCaptureImpl {
public:
    bool Initialize(int displayIndex, int targetFps) {
        (void)targetFps;
        winrt::init_apartment(winrt::apartment_type::multi_threaded);

        UINT createDeviceFlags = D3D11_CREATE_DEVICE_VIDEO_SUPPORT;
        D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
        D3D_FEATURE_LEVEL featureLevel;
        HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
            featureLevels, 2, D3D11_SDK_VERSION, &m_device, &featureLevel, &m_context);
        if (FAILED(hr)) return false;

        ComPtr<IDXGIDevice> dxgiDevice;
        hr = m_device.As(&dxgiDevice);
        if (FAILED(hr)) return false;

        winrt::com_ptr<::IInspectable> inspectable;
        hr = CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.Get(), inspectable.put());
        if (FAILED(hr)) return false;
        m_winrtDevice = inspectable.as<winrt_d3d11::IDirect3DDevice>();

        ComPtr<IDXGIAdapter> dxgiAdapter;
        dxgiDevice->GetAdapter(&dxgiAdapter);
        ComPtr<IDXGIOutput> dxgiOutput;
        hr = dxgiAdapter->EnumOutputs(displayIndex, &dxgiOutput);
        if (FAILED(hr)) return false;

        auto factory = winrt::get_activation_factory<winrt_capture::GraphicsCaptureItem>();
        auto interop = factory.as<IGraphicsCaptureItemInterop>();
        
        HMONITOR hmon = nullptr;
        DXGI_OUTPUT_DESC desc;
        dxgiOutput->GetDesc(&desc);
        hmon = desc.Monitor;

        hr = interop->CreateForMonitor(hmon, winrt::guid_of<winrt_capture::GraphicsCaptureItem>(), reinterpret_cast<void**>(winrt::put_abi(m_item)));
        if (FAILED(hr) || !m_item) return false;

        // Instanciar FramePool conforme o Lote 1 da especificacao
        m_framePool = winrt_capture::Direct3D11CaptureFramePool::CreateFreeThreaded(
            m_winrtDevice,
            winrt_directx::DirectXPixelFormat::B8G8R8A8UIntNormalized,
            2,
            m_item.Size()
        );

        m_session = m_framePool.CreateCaptureSession(m_item);
        
        // Desativar borda amarela de captura se suportado
        try {
            m_session.IsBorderRequired(false);
        } catch (...) {}

        m_frameArrivedToken = m_framePool.FrameArrived({ this, &WGCCaptureImpl::OnFrameArrived });

        m_session.StartCapture();
        return true;
    }

    bool AcquireFrame(ID3D11Texture2D** ppTexture, uint64_t& timestampUs) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_latestTexture) {
            *ppTexture = m_latestTexture.Get();
            (*ppTexture)->AddRef();
            timestampUs = m_latestTimestamp;
            return true;
        }
        return false;
    }

    void ReleaseFrame() {}

    ID3D11Device* GetDevice() { return m_device.Get(); }

    ~WGCCaptureImpl() {
        if (m_session) m_session.Close();
        if (m_framePool) {
            m_framePool.FrameArrived(m_frameArrivedToken);
            m_framePool.Close();
        }
    }

private:
    void OnFrameArrived(winrt_capture::Direct3D11CaptureFramePool const& sender, winrt::Windows::Foundation::IInspectable const& /*args*/) {
        auto frame = sender.TryGetNextFrame();
        if (!frame) return;

        ComPtr<ID3D11Texture2D> tex;
        auto surface = frame.Surface();
        auto access = surface.as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
        HRESULT hr = access->GetInterface(IID_PPV_ARGS(&tex));
        
        if (SUCCEEDED(hr)) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_latestTexture = tex;
            LARGE_INTEGER qpc;
            QueryPerformanceCounter(&qpc);
            m_latestTimestamp = qpc.QuadPart;
        }
    }

    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_context;
    winrt_d3d11::IDirect3DDevice m_winrtDevice{ nullptr };
    winrt_capture::GraphicsCaptureItem m_item{ nullptr };
    winrt_capture::Direct3D11CaptureFramePool m_framePool{ nullptr };
    winrt_capture::GraphicsCaptureSession m_session{ nullptr };
    winrt::event_token m_frameArrivedToken;

    std::mutex m_mutex;
    ComPtr<ID3D11Texture2D> m_latestTexture;
    uint64_t m_latestTimestamp = 0;
};

WGCCapture::WGCCapture() : m_impl(new WGCCaptureImpl()) {}
WGCCapture::~WGCCapture() { delete m_impl; }
bool WGCCapture::Initialize(int displayIndex, int targetFps) { return m_impl->Initialize(displayIndex, targetFps); }
bool WGCCapture::AcquireFrame(ID3D11Texture2D** ppTexture, uint64_t& timestampUs) { return m_impl->AcquireFrame(ppTexture, timestampUs); }
void WGCCapture::ReleaseFrame() {
    if (m_impl) m_impl->ReleaseFrame();
}

ID3D11Device* WGCCapture::GetDevice() {
    return m_impl ? m_impl->GetDevice() : nullptr;
}
