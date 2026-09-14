#include "client/D3D11VaDecoder.h"
#include "utils/Logger.h"
#include <d3d11.h>
#include <iostream>

#pragma comment(lib, "d3d11.lib")

int main() {
    ID3D11Device* device = nullptr;
    D3D_FEATURE_LEVEL featureLevel;
    ID3D11DeviceContext* context = nullptr;
    D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &device, &featureLevel, &context);
    
    if(!device) {
        std::cout << "Failed to create d3d device" << std::endl;
        return 1;
    }

    D3D11VaDecoder decoder;
    if(decoder.Initialize(device)) {
        std::cout << "Decoder initialized successfully!" << std::endl;
    } else {
        std::cout << "Decoder failed to initialize!" << std::endl;
    }
    return 0;
}
