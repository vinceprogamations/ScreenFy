#pragma once
#include <d3d11.h>
#include <mfapi.h>
#include <mftransform.h>
#include <wrl/client.h>
#include <vector>
#include <cstdint>

class D3D11VaDecoder {
public:
    D3D11VaDecoder();
    ~D3D11VaDecoder();

    bool Initialize(ID3D11Device* pDevice);
    bool DecodeNAL(const std::vector<uint8_t>& nalUnit);
    bool GetDecodedTexture(ID3D11Texture2D** ppTexture);
    bool GetDecodedTextureSRV(ID3D11ShaderResourceView** ppSRV);

private:
    Microsoft::WRL::ComPtr<IMFTransform> m_decoder;
    Microsoft::WRL::ComPtr<IMFDXGIDeviceManager> m_devManager;
    Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_srv;
};
