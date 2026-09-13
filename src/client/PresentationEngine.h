#pragma once
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <windows.h>

class PresentationEngine {
public:
    PresentationEngine();
    ~PresentationEngine();

    bool Initialize(HWND hWnd);
    void Render();
    
    ID3D11Device* GetDevice() { return m_device.Get(); }
    ID3D11DeviceContext* GetContext() { return m_context.Get(); }
    IDXGISwapChain1* GetSwapChain() { return m_swapChain.Get(); }

private:
    Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapChain;
};
