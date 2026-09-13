#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include <string>
#include <unordered_map>
#include <mutex>

class TextureLoader {
public:
    static TextureLoader& Get();

    void Initialize(ID3D11Device* device, ID3D11DeviceContext* context);

    // Carrega imagem de arquivo para ShaderResourceView
    ID3D11ShaderResourceView* LoadTextureFromFile(const std::string& filePath);

    // Abre a janela nativa do Windows para selecionar imagem (PNG, JPG, BMP)
    static std::string OpenImageFileDialog(HWND parentHwnd = nullptr);

    // Limpar cache de texturas
    void ClearCache();

private:
    TextureLoader() = default;
    ~TextureLoader() = default;
    TextureLoader(const TextureLoader&) = delete;
    TextureLoader& operator=(const TextureLoader&) = delete;

    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
    std::mutex m_mutex;
    std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>> m_cache;
};
