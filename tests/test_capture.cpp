#include "../src/capture/CaptureEngine.h"
#include <iostream>
#include <windows.h>

int main() {
    std::cout << "Starting Capture Test (Lote 1)...\n";
    
    CaptureEngine engine;
    // Tenta capturar o ecrã principal (0) com target 120 FPS
    if (!engine.Initialize(0, 120)) {
        std::cerr << "Failed to initialize CaptureEngine.\n";
        return -1;
    }
    
    std::cout << "CaptureEngine initialized. Starting 10-second capture loop at 120 FPS target...\n";
    
    const int targetFrames = 1200; // 120 FPS * 10 segundos
    int framesAcquired = 0;
    
    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    
    ID3D11Texture2D* pTexture = nullptr;
    uint64_t timestamp = 0;
    
    // Ciclo de aquisição de 10 segundos
    while (framesAcquired < targetFrames) {
        if (engine.AcquireFrame(&pTexture, timestamp)) {
            framesAcquired++;
            engine.ReleaseFrame();
        } else {
            // Prevenir 100% uso CPU em caso de falha transitória
            Sleep(1);
        }
    }
    
    QueryPerformanceCounter(&end);
    
    double elapsedSeconds = static_cast<double>(end.QuadPart - start.QuadPart) / freq.QuadPart;
    double effectiveFps = framesAcquired / elapsedSeconds;
    
    std::cout << "\n--- Resultados do Teste ---\n";
    std::cout << "Frames Capturados: " << framesAcquired << "\n";
    std::cout << "Tempo Decorrido:   " << elapsedSeconds << " s\n";
    std::cout << "FPS Efetivos:      " << effectiveFps << "\n";
    
    if (effectiveFps >= 118.0) {
        std::cout << "SUCESSO: A captura atingiu a meta de estabilidade a 120 FPS (>118 FPS).\n";
        return 0;
    } else {
        std::cerr << "FALHA: A captura não manteve os 120 FPS exigidos (resultado foi menor que 118 FPS).\n";
        return -1;
    }
}
