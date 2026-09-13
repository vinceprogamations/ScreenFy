#pragma once
#include "ICaptureSource.h"
#include <memory>

class CaptureEngine : public ICaptureSource {
public:
    CaptureEngine();
    ~CaptureEngine() override;

    bool Initialize();
    bool Initialize(int displayIndex, int targetFps) override;
    bool Initialize(int displayIndex, int targetFps, bool useWGCFallback);
    bool AcquireFrame(ID3D11Texture2D** ppTexture, uint64_t& timestampUs) override;
    void ReleaseFrame() override;
    ID3D11Device* GetDevice() override;

private:
    std::unique_ptr<ICaptureSource> m_source;
};
