cmake_minimum_required(VERSION 3.25)
project(ScreenShare4K VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Otimizações de alta performance para MSVC
if(MSVC)
    add_compile_options(/O2 /Oi /GL /W4)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# Definir diretórios de headers
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/thirdparty/nvenc)

find_package(Opus CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Fontes de Identidade (Lote 5)
set(IDENTITY_SOURCES
    src/identity/FriendManager.cpp
)

# Fontes do Capture
set(CAPTURE_SOURCES
    src/capture/DXGICapture.cpp
    src/capture/WGCCapture.cpp
    src/capture/CaptureEngine.cpp
)

set(ENCODER_SOURCES
    src/encoder/ColorConverterD3D11.cpp
    src/encoder/VideoEncoderNVENC.cpp
    src/encoder/VideoEncoderAMD.cpp
    src/encoder/UniversalVideoEncoder.cpp
)

set(AUDIO_SOURCES
    src/audio/WasapiLoopback.cpp
    src/audio/OpusAudioEncoder.cpp
)

set(NETWORK_SOURCES
    src/network/RtpPacketizer.cpp
    src/network/ReedSolomonFec.cpp
    src/network/UdpTransport.cpp
)

set(SIGNALING_SOURCES
    src/network/SignalingClient.cpp
)

set(CLIENT_SOURCES
    src/client/D3D11VaDecoder.cpp
    src/client/PresentationEngine.cpp
)

set(ENGINE_SOURCES
    src/engine/StreamEngine.cpp
    src/engine/StreamHealthMonitor.cpp
)

set(CONFIG_SOURCES
    src/config/SettingsManager.cpp
)

set(UTILS_SOURCES
    src/utils/Logger.cpp
)

set(UI_SOURCES
    src/ui/AppWindow.cpp
    src/ui/Sidebar.cpp
    src/ui/ChatPanel.cpp
    src/ui/OverlayCall.cpp
    src/ui/ConnectionOverlay.cpp
    src/ui/SettingsPanel.cpp
    src/ui/GoLiveModal.cpp
    src/ui/ScreenTestModal.cpp
    src/ui/TextureLoader.cpp
    src/capture/ScreenPreviewer.cpp
    src/main.cpp
)

# App Principal Integrada
add_executable(ScreenShareApp WIN32 ${UI_SOURCES} ${CLIENT_SOURCES} ${ENGINE_SOURCES} ${CAPTURE_SOURCES} ${ENCODER_SOURCES} ${AUDIO_SOURCES} ${NETWORK_SOURCES} ${SIGNALING_SOURCES} ${IDENTITY_SOURCES} ${CONFIG_SOURCES} ${UTILS_SOURCES})
target_link_libraries(ScreenShareApp PRIVATE d3d11.lib dxgi.lib windowsapp.lib d3dcompiler.lib ws2_32.lib iphlpapi.lib winmm.lib gdi32.lib user32.lib mfplat.lib mfuuid.lib mf.lib mfreadwrite.lib windowscodecs.lib comdlg32.lib Opus::opus imgui::imgui nlohmann_json::nlohmann_json)

# Teste unitário Captura
add_executable(test_capture tests/test_capture.cpp ${CAPTURE_SOURCES} ${CONFIG_SOURCES} ${IDENTITY_SOURCES} ${UTILS_SOURCES})
target_link_libraries(test_capture PRIVATE d3d11.lib dxgi.lib windowsapp.lib winmm.lib iphlpapi.lib ws2_32.lib nlohmann_json::nlohmann_json)

# Teste unitário Encoder + Áudio
add_executable(test_encoder tests/test_encoder.cpp ${CAPTURE_SOURCES} ${ENCODER_SOURCES} ${AUDIO_SOURCES} ${CONFIG_SOURCES} ${IDENTITY_SOURCES} ${UTILS_SOURCES})
target_link_libraries(test_encoder PRIVATE d3d11.lib dxgi.lib windowsapp.lib d3dcompiler.lib winmm.lib iphlpapi.lib ws2_32.lib mfplat.lib mfuuid.lib mf.lib mfreadwrite.lib Opus::opus nlohmann_json::nlohmann_json)

# Teste unitário Rede
add_executable(test_network tests/test_network.cpp ${NETWORK_SOURCES} ${CONFIG_SOURCES} ${IDENTITY_SOURCES} ${UTILS_SOURCES})
target_link_libraries(test_network PRIVATE ws2_32.lib winmm.lib iphlpapi.lib nlohmann_json::nlohmann_json)

# Teste unitário Configurações (Lote 6)
add_executable(test_settings tests/test_settings.cpp ${CONFIG_SOURCES} ${IDENTITY_SOURCES} ${NETWORK_SOURCES} ${UTILS_SOURCES})
target_link_libraries(test_settings PRIVATE ws2_32.lib winmm.lib iphlpapi.lib nlohmann_json::nlohmann_json)

# Registar o teste para o CTest
enable_testing()
add_test(NAME test_capture COMMAND test_capture)
add_test(NAME test_encoder COMMAND test_encoder)
add_test(NAME test_network COMMAND test_network)
add_test(NAME test_settings COMMAND test_settings)
