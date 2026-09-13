#include "../src/capture/CaptureEngine.h"
#include "../src/encoder/ColorConverterD3D11.h"
#include "../src/encoder/UniversalVideoEncoder.h"
#include "../src/audio/WasapiLoopback.h"
#include "../src/audio/OpusAudioEncoder.h"

#include <iostream>
#include <windows.h>
#include <thread>
#include <atomic>

int main() {
    std::cout << "Starting Encoder Test (Universal Multi-GPU: AMD / NVIDIA)...\n";
    
    CaptureEngine engine;
    if (!engine.Initialize(0, 120)) {
        std::cerr << "Falha ao iniciar CaptureEngine.\n";
        return -1;
    }
    
    ID3D11Device* pDevice = engine.GetDevice();
    if (!pDevice) {
        std::cerr << "Falha ao obter ID3D11Device da Captura.\n";
        return -1;
    }

    std::cout << "A obter o primeiro frame para detetar resolucao...\n";
    ID3D11Texture2D* pFirstTex = nullptr;
    uint64_t ts = 0;
    while (!engine.AcquireFrame(&pFirstTex, ts)) Sleep(10);
    
    D3D11_TEXTURE2D_DESC desc;
    pFirstTex->GetDesc(&desc);
    engine.ReleaseFrame();

    int width = desc.Width;
    int height = desc.Height;
    std::cout << "Resolucao: " << width << "x" << height << "\n";

    ColorConverterD3D11 colorConverter;
    if (!colorConverter.Initialize(pDevice, width, height)) {
        std::cerr << "Falha ColorConverterD3D11.\n";
        return -1;
    }

    UniversalVideoEncoder encoder;
    if (!encoder.Initialize(pDevice, width, height)) {
        std::cerr << "Falha UniversalVideoEncoder Initialize.\n";
        return -1;
    }

    std::cout << "GPU Detectada: " << encoder.GetGpuName() << "\n";
    std::cout << "Codificador Ativo: " << encoder.GetEncoderName() << "\n";

    WasapiLoopback audioLoopback;
    if (!audioLoopback.Initialize()) {
        std::cerr << "Falha WASAPI Initialize.\n";
        return -1;
    }

    OpusAudioEncoder opusEncoder;
    if (!opusEncoder.Initialize(48000, 2, 128000)) {
        std::cerr << "Falha Opus Initialize.\n";
        return -1;
    }

    std::cout << "Pipelines iniciados. A capturar e codificar durante 5 segundos...\n";
    
    std::atomic<bool> keepRunning{true};
    
    auto audioThread = std::thread([&]() {
        while(keepRunning) {
            std::vector<float> pcm;
            if (audioLoopback.CaptureAudio(pcm)) {
                std::vector<std::vector<uint8_t>> packets;
                opusEncoder.Encode(pcm, packets);
            }
            Sleep(10);
        }
    });

    const int targetFrames = 600; // 5 segundos a 120fps
    int framesAcquired = 0;
    
    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);
    
    ID3D11Texture2D* pTexture = nullptr;
    uint64_t timestamp = 0;
    
    double totalEncodeTimeMs = 0.0;

    while (framesAcquired < targetFrames) {
        if (engine.AcquireFrame(&pTexture, timestamp)) {
            
            ID3D11Texture2D* pNV12Texture = nullptr;
            
            QueryPerformanceCounter(&start);
            if (colorConverter.Convert(pTexture, &pNV12Texture)) {
                std::vector<std::vector<uint8_t>> nalUnits;
                encoder.EncodeFrame(pNV12Texture, nalUnits);
                pNV12Texture->Release();
            }
            QueryPerformanceCounter(&end);
            
            double encodeTime = static_cast<double>(end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart;
            totalEncodeTimeMs += encodeTime;

            framesAcquired++;
            engine.ReleaseFrame();
        } else {
            Sleep(1);
        }
    }
    
    keepRunning = false;
    audioThread.join();

    double avgEncodeTime = totalEncodeTimeMs / (framesAcquired > 0 ? framesAcquired : 1);

    std::cout << "\n--- Resultados do Codificador ---\n";
    std::cout << "Frames Codificados: " << framesAcquired << "\n";
    std::cout << "Tempo medio de servico (Color + Hardware Encoder): " << avgEncodeTime << " ms\n";
    
    if (avgEncodeTime < 5.0) {
        std::cout << "SUCESSO: Codificador de Hardware respeitou o orcamento de baixa latencia.\n";
        return 0;
    } else {
        std::cerr << "FALHA: Latencia do codificador excedeu os 5.0ms.\n";
        return -1;
    }
}
