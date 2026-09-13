#include "ui/AppWindow.h"
#include "client/PresentationEngine.h"
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    // Lote 7: Ativar DPI Awareness da janela antes de qualquer inicialização gráfica
    ImGui_ImplWin32_EnableDpiAwareness();

    AppWindow app;
    // Janela estilo "App" Discord (1080x720 base, auto-escalada com DPI)
    if (!app.Initialize(1080, 720, "ScreenShare4K")) return -1;

    PresentationEngine engine;
    if (!engine.Initialize(app.GetHWND())) return -1;

    app.SetD3D11(engine.GetDevice(), engine.GetContext());
    ImGui_ImplDX11_Init(engine.GetDevice(), engine.GetContext());

    while (app.ProcessMessages()) {
        app.RenderUI();

        Microsoft::WRL::ComPtr<ID3D11Texture2D> pBackBuffer;
        engine.GetSwapChain()->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
        
        Microsoft::WRL::ComPtr<ID3D11RenderTargetView> pRTV;
        engine.GetDevice()->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &pRTV);
        
        // Fundo estilo escuro Discord
        const float clearColor[4] = { 0.17f, 0.18f, 0.19f, 1.0f };
        engine.GetContext()->ClearRenderTargetView(pRTV.Get(), clearColor);
        engine.GetContext()->OMSetRenderTargets(1, pRTV.GetAddressOf(), nullptr);

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        
        engine.Render();
    }

    ImGui_ImplDX11_Shutdown();
    return 0;
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOWNORMAL);
}
