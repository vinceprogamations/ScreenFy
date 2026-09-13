#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d11.h>
#include <string>
#include <thread>
#include <atomic>
#include <memory>
#include <vector>

class CaptureEngine;
class UniversalVideoEncoder;
class UdpTransport;
class RtpPacketizer;
class D3D11VaDecoder;

class StreamEngine {
public:
    static StreamEngine& Instance();

    bool Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    
    // Inicia fluxo de envio (Host)
    bool StartHosting(const std::string& targetIp, int targetPort);
    
    // Inicia fluxo de receção (Cliente)
    bool StartReceiving(const std::string& targetIp, int listenPort);
    
    void Stop();

    bool IsHosting() const { return m_isHosting; }
    bool IsReceiving() const { return m_isReceiving; }

    ID3D11ShaderResourceView* GetReceivedVideoSRV() const;

private:
    StreamEngine();
    ~StreamEngine();
    StreamEngine(const StreamEngine&) = delete;
    StreamEngine& operator=(const StreamEngine&) = delete;

    void SendLoop();
    void ReceiveLoop();

    ID3D11Device* m_pDevice = nullptr;
    ID3D11DeviceContext* m_pContext = nullptr;

    std::atomic<bool> m_running{ false };
    std::atomic<bool> m_isHosting{ false };
    std::atomic<bool> m_isReceiving{ false };

    std::thread m_workerThread;

    std::unique_ptr<CaptureEngine> m_capture;
    std::unique_ptr<UniversalVideoEncoder> m_encoder;
    std::unique_ptr<UdpTransport> m_transport;
    std::unique_ptr<RtpPacketizer> m_packetizer;
    std::unique_ptr<D3D11VaDecoder> m_decoder;

    std::string m_targetIp;
    int m_targetPort = 50000;
};
