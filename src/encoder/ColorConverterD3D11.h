#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <cstdint>

class ColorConverterD3D11 {
public:
    ColorConverterD3D11();
    ~ColorConverterD3D11();

    bool Initialize(ID3D11Device* device, int width, int height);
    bool Convert(ID3D11Texture2D* pInputBGRA, ID3D11Texture2D** ppOutputNV12);

private:
    Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
    
    Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_computeShader;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> m_nv12Texture;
    
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_srv;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_uavY;
    Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_uavUV;

    int m_width;
    int m_height;
};
