#include "TextureLoader.h"
#include "../utils/Logger.h"
#include <wincodec.h>
#include <commdlg.h>
#include <vector>
#include <iostream>

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "comdlg32.lib")

using Microsoft::WRL::ComPtr;

TextureLoader& TextureLoader::Get() {
    static TextureLoader instance;
    return instance;
}

void TextureLoader::Initialize(ID3D11Device* device, ID3D11DeviceContext* context) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_device = device;
    m_context = context;
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
}

void TextureLoader::ClearCache() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache.clear();
}

std::string TextureLoader::OpenImageFileDialog(HWND parentHwnd) {
    char szFile[MAX_PATH] = { 0 };

    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = parentHwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Imagens (*.png;*.jpg;*.jpeg;*.bmp)\0*.png;*.jpg;*.jpeg;*.bmp\0Todos os Ficheiros (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn) == TRUE) {
        return std::string(szFile);
    }
    return "";
}

ID3D11ShaderResourceView* TextureLoader::LoadTextureFromFile(const std::string& filePath) {
    if (filePath.empty() || !m_device) return nullptr;

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_cache.find(filePath);
    if (it != m_cache.end() && it->second) {
        return it->second.Get();
    }

    // Inicializar WIC Factory
    ComPtr<IWICImagingFactory> pWICFactory;
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&pWICFactory)
    );

    if (FAILED(hr)) {
        LOG_ERROR("TEXTURE", "Falha ao criar IWICImagingFactory (HRESULT: " + std::to_string(hr) + ")");
        return nullptr;
    }

    // Converter path UTF-8 para wide string (suporte a acentos/unicode)
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, filePath.c_str(), -1, NULL, 0);
    std::wstring wPath(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, filePath.c_str(), -1, &wPath[0], size_needed);

    ComPtr<IWICBitmapDecoder> pDecoder;
    hr = pWICFactory->CreateDecoderFromFilename(
        wPath.c_str(),
        nullptr,
        GENERIC_READ,
        WICDecodeMetadataCacheOnDemand,
        &pDecoder
    );

    if (FAILED(hr)) {
        LOG_WARN("TEXTURE", "Falha ao carregar ficheiro de imagem: " + filePath);
        return nullptr;
    }

    ComPtr<IWICBitmapFrameDecode> pFrame;
    hr = pDecoder->GetFrame(0, &pFrame);
    if (FAILED(hr)) {
        LOG_WARN("TEXTURE", "Falha ao decodificar frame de: " + filePath);
        return nullptr;
    }

    UINT width = 0, height = 0;
    pFrame->GetSize(&width, &height);
    if (width == 0 || height == 0) return nullptr;

    ComPtr<IWICFormatConverter> pConverter;
    hr = pWICFactory->CreateFormatConverter(&pConverter);
    if (FAILED(hr)) return nullptr;

    hr = pConverter->Initialize(
        pFrame.Get(),
        GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone,
        nullptr,
        0.0f,
        WICBitmapPaletteTypeCustom
    );
    if (FAILED(hr)) return nullptr;

    UINT stride = width * 4;
    UINT bufferSize = stride * height;
    std::vector<BYTE> pixelBuffer(bufferSize);

    hr = pConverter->CopyPixels(nullptr, stride, bufferSize, pixelBuffer.data());
    if (FAILED(hr)) return nullptr;

    // Criar D3D11 Texture2D
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA subData = {};
    subData.pSysMem = pixelBuffer.data();
    subData.SysMemPitch = stride;

    ComPtr<ID3D11Texture2D> pTexture;
    hr = m_device->CreateTexture2D(&desc, &subData, &pTexture);
    if (FAILED(hr)) {
        LOG_ERROR("TEXTURE", "Falha ao criar Texture2D para " + filePath);
        return nullptr;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    ComPtr<ID3D11ShaderResourceView> pSRV;
    hr = m_device->CreateShaderResourceView(pTexture.Get(), &srvDesc, &pSRV);
    if (FAILED(hr)) {
        LOG_ERROR("TEXTURE", "Falha ao criar ShaderResourceView para " + filePath);
        return nullptr;
    }

    ID3D11ShaderResourceView* rawSRV = pSRV.Get();
    m_cache[filePath] = pSRV;
    LOG_INFO("TEXTURE", "Avatar carregado com sucesso: " + filePath + " (" + std::to_string(width) + "x" + std::to_string(height) + ")");
    return rawSRV;
}
