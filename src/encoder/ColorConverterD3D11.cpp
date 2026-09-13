#include "ColorConverterD3D11.h"
#include <d3dcompiler.h>
#include <iostream>
#include <string>

// HLSL Shader para conversão de BGRA (Textura nativa do D3D11/DXGI) -> NV12
static const char* bgra_to_nv12_hlsl = R"(
Texture2D<unorm float4> InputTexture : register(t0);
RWTexture2D<unorm float> OutputY : register(u0);
RWTexture2D<unorm float2> OutputUV : register(u1);

[numthreads(8, 8, 1)]
void CS_Main(uint3 DTid : SV_DispatchThreadID) {
    uint x = DTid.x;
    uint y = DTid.y;
    
    uint width, height;
    InputTexture.GetDimensions(width, height);
    if (x >= width || y >= height) return;

    // A cor em DXGI_FORMAT_B8G8R8A8_UNORM pode vir em ordem inversa. Mapear corretamente.
    float4 color = InputTexture[uint2(x, y)];
    
    float R = color.r;
    float G = color.g;
    float B = color.b;
    
    // Conversão BT.709
    float Y = 0.2126 * R + 0.7152 * G + 0.0722 * B;
    float U = -0.1146 * R - 0.3854 * G + 0.5000 * B + 0.5;
    float V = 0.5000 * R - 0.4542 * G - 0.0458 * B + 0.5;
    
    OutputY[uint2(x, y)] = Y;
    
    // Sub-amostragem 4:2:0
    if ((x % 2 == 0) && (y % 2 == 0)) {
        OutputUV[uint2(x / 2, y / 2)] = float2(U, V);
    }
}
)";

ColorConverterD3D11::ColorConverterD3D11() = default;
ColorConverterD3D11::~ColorConverterD3D11() = default;

bool ColorConverterD3D11::Initialize(ID3D11Device* device, int width, int height) {
    m_device = device;
    m_device->GetImmediateContext(&m_context);
    m_width = width;
    m_height = height;

    Microsoft::WRL::ComPtr<ID3DBlob> shaderBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    
    HRESULT hr = D3DCompile(
        bgra_to_nv12_hlsl, strlen(bgra_to_nv12_hlsl),
        "BGRA2NV12", nullptr, nullptr, "CS_Main", "cs_5_0",
        0, 0, &shaderBlob, &errorBlob
    );

    if (FAILED(hr)) {
        if (errorBlob) {
            std::cerr << "Shader compile error: " << (char*)errorBlob->GetBufferPointer() << "\n";
        }
        return false;
    }

    hr = m_device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &m_computeShader);
    if (FAILED(hr)) return false;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_NV12;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    
    hr = m_device->CreateTexture2D(&desc, nullptr, &m_nv12Texture);
    if (FAILED(hr)) {
        std::cerr << "Failed to create NV12 texture. HR: " << std::hex << hr << std::endl;
        return false;
    }

    // Criar as views para os planos Y (R8) e UV (R8G8)
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
    
    uavDesc.Format = DXGI_FORMAT_R8_UNORM;
    hr = m_device->CreateUnorderedAccessView(m_nv12Texture.Get(), &uavDesc, &m_uavY);
    if (FAILED(hr)) return false;

    uavDesc.Format = DXGI_FORMAT_R8G8_UNORM;
    hr = m_device->CreateUnorderedAccessView(m_nv12Texture.Get(), &uavDesc, &m_uavUV);
    if (FAILED(hr)) return false;

    return true;
}

bool ColorConverterD3D11::Convert(ID3D11Texture2D* pInputBGRA, ID3D11Texture2D** ppOutputNV12) {
    if (!m_computeShader || !pInputBGRA) return false;

    if (!m_srv) {
        D3D11_TEXTURE2D_DESC inDesc;
        pInputBGRA->GetDesc(&inDesc);

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Format = inDesc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        
        HRESULT hr = m_device->CreateShaderResourceView(pInputBGRA, &srvDesc, &m_srv);
        if (FAILED(hr)) return false;
    }

    m_context->CSSetShader(m_computeShader.Get(), nullptr, 0);
    
    ID3D11ShaderResourceView* srvs[] = { m_srv.Get() };
    m_context->CSSetShaderResources(0, 1, srvs);

    ID3D11UnorderedAccessView* uavs[] = { m_uavY.Get(), m_uavUV.Get() };
    m_context->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);

    m_context->Dispatch((m_width + 7) / 8, (m_height + 7) / 8, 1);

    ID3D11ShaderResourceView* nullSRV[] = { nullptr };
    m_context->CSSetShaderResources(0, 1, nullSRV);
    
    ID3D11UnorderedAccessView* nullUAV[] = { nullptr, nullptr };
    m_context->CSSetUnorderedAccessViews(0, 2, nullUAV, nullptr);

    *ppOutputNV12 = m_nv12Texture.Get();
    (*ppOutputNV12)->AddRef();
    
    return true;
}
