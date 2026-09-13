#pragma once
#include <d3d11.h>
#include <cstdint>

class ICaptureSource {
public:
    virtual ~ICaptureSource() = default;
    virtual bool Initialize(int displayIndex, int targetFps) = 0;
    virtual bool AcquireFrame(ID3D11Texture2D** ppTexture, uint64_t& timestampUs) = 0;
    virtual void ReleaseFrame() = 0;
    virtual ID3D11Device* GetDevice() = 0;
};
