#pragma once
#include "ICaptureSource.h"
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class DXGICapture : public ICaptureSource {
public:
    DXGICapture();
    ~DXGICapture() override;

    bool Initialize(int displayIndex, int targetFps) override;
    bool AcquireFrame(ID3D11Texture2D** ppTexture, uint64_t& timestampUs) override;
    void ReleaseFrame() override;
    ID3D11Device* GetDevice() override { return m_device.Get(); }

private:
    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_context;
    ComPtr<IDXGIOutputDuplication> m_deskDupl;
    ComPtr<ID3D11Texture2D> m_lastFrame;
    int m_targetFps;
};
