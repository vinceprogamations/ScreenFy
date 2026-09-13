#pragma once
#include "ICaptureSource.h"

class WGCCaptureImpl;

class WGCCapture : public ICaptureSource {
public:
    WGCCapture();
    ~WGCCapture() override;

    bool Initialize(int displayIndex, int targetFps) override;
    bool AcquireFrame(ID3D11Texture2D** ppTexture, uint64_t& timestampUs) override;
    void ReleaseFrame() override;
    ID3D11Device* GetDevice() override;

private:
    WGCCaptureImpl* m_impl;
};
