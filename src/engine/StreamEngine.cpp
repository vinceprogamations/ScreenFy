#include "StreamEngine.h"
#include "StreamHealthMonitor.h"
#include "../capture/CaptureEngine.h"
#include "../encoder/UniversalVideoEncoder.h"
#include "../network/UdpTransport.h"
#include "../network/RtpPacketizer.h"
#include "../client/D3D11VaDecoder.h"
#include "../utils/Logger.h"
#include "../network/SignalingClient.h"

StreamEngine& StreamEngine::Instance() {
    static StreamEngine instance;
    return instance;
}

StreamEngine::StreamEngine() = default;

StreamEngine::~StreamEngine() {
    Stop();
}

bool StreamEngine::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext) {
    m_pDevice = pDevice;
    m_pContext = pContext;
    return true;
}

bool StreamEngine::StartHosting(const std::string& targetIp, int targetPort) {
    Stop();
    
    m_targetIp = targetIp;
    m_targetPort = targetPort;
    
    StreamHealthMonitor::Instance().Reset();
    StreamHealthMonitor::Instance().SetStatus(HealthStep::NotStarted);
    
    m_capture = std::make_unique<CaptureEngine>();
    if (!m_capture->Initialize(0, 120)) {
        LOG_ERROR("STREAM", "Falha ao iniciar CaptureEngine");
        StreamHealthMonitor::Instance().Fail("DXGI Capture Falhou");
        return false;
    }
    
    m_encoder = std::make_unique<UniversalVideoEncoder>();
    if (!m_encoder->Initialize(m_pDevice, 1920, 1080, 5000)) {
        LOG_ERROR("STREAM", "Falha ao iniciar VideoEncoder");
        StreamHealthMonitor::Instance().Fail("NVENC/AMF Inicialização Falhou");
        return false;
    }

    m_transport = std::make_unique<UdpTransport>();
    if (!m_transport->Initialize("", 0, targetIp, static_cast<uint16_t>(targetPort))) {
        LOG_ERROR("STREAM", "Falha ao iniciar UdpTransport (Host)");
        StreamHealthMonitor::Instance().Fail("UDP Socket Bind Falhou");
        return false;
    }
    
    m_packetizer = std::make_unique<RtpPacketizer>(12345, 96);
    
    m_running = true;
    m_isHosting = true;
    m_workerThread = std::thread(&StreamEngine::SendLoop, this);
    
    LOG_INFO("STREAM", "Host Loop Iniciado para " + targetIp);
    return true;
}

bool StreamEngine::StartReceiving(const std::string& targetIp, int listenPort) {
    if (m_running) Stop();
    
    StreamHealthMonitor::Instance().Reset();
    StreamHealthMonitor::Instance().SetStatus(HealthStep::NotStarted);

    m_targetIp = targetIp;
    m_transport = std::make_unique<UdpTransport>();
    // Binding local com IP qualquer (0.0.0.0) na porta especificada
    if (!m_transport->Initialize("0.0.0.0", static_cast<uint16_t>(listenPort), "", 0)) {
        LOG_ERROR("STREAM", "Falha ao iniciar UdpTransport (Receive)");
        StreamHealthMonitor::Instance().Fail("UDP Bind Falhou");
        return false;
    }
    
    m_decoder = std::make_unique<D3D11VaDecoder>();
    if (!m_decoder->Initialize(m_pDevice)) {
        LOG_ERROR("STREAM", "Falha ao iniciar D3D11VaDecoder");
        StreamHealthMonitor::Instance().Fail("Decoder MFT Falhou");
        return false;
    }

    m_running = true;
    m_isReceiving = true;
    m_workerThread = std::thread(&StreamEngine::ReceiveLoop, this);
    
    LOG_INFO("STREAM", "Receive Loop Iniciado na porta " + std::to_string(listenPort));
    return true;
}

void StreamEngine::Stop() {
    m_running = false;
    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
    m_isHosting = false;
    m_isReceiving = false;
    
    m_capture.reset();
    m_encoder.reset();
    m_transport.reset();
    m_packetizer.reset();
    m_decoder.reset();
}

ID3D11ShaderResourceView* StreamEngine::GetReceivedVideoSRV() const {
    if (m_decoder) {
        ID3D11ShaderResourceView* srv = nullptr;
        m_decoder->GetDecodedTextureSRV(&srv); // Nota: Presume-se q implementaremos GetDecodedTextureSRV
        return srv;
    }
    return nullptr;
}

void StreamEngine::SendLoop() {
    uint64_t timestamp = 0;
    ID3D11Texture2D* pTexture = nullptr;
    int frameCount = 0;
    
    // Simular Ping UDP para Etapa 3 (NetworkRoute)
    std::string pingMsg = "PING";
    m_transport->Send(reinterpret_cast<const uint8_t*>(pingMsg.data()), pingMsg.size());
    StreamHealthMonitor::Instance().HookUdpPing(true);
    
    while (m_running) {
        if (m_capture->AcquireFrame(&pTexture, timestamp)) {
            // Hook 1: Frame capturado
            if (!StreamHealthMonitor::Instance().HookCaptureFrame(pTexture, m_pContext)) {
                // Heurística Ecrã preto -> WGC fallback (simplificado para o plano)
            }
            
            std::vector<std::vector<uint8_t>> nalUnits;
            if (m_encoder->EncodeFrame(pTexture, nalUnits)) {
                
                size_t totalBytes = 0;
                for (const auto& nal : nalUnits) totalBytes += nal.size();
                
                // Hook 2: Encoder cospe bytes
                StreamHealthMonitor::Instance().HookEncoderOutput(totalBytes, frameCount);
                
                for (const auto& nal : nalUnits) {
                    auto packets = m_packetizer->Packetize(nal, (uint32_t)timestamp, true);
                    for (const auto& pkt : packets) {
                        std::vector<uint8_t> udpBuf;
                        udpBuf.insert(udpBuf.end(), (uint8_t*)&pkt.header, (uint8_t*)&pkt.header + sizeof(RtpHeader));
                        udpBuf.insert(udpBuf.end(), pkt.payload.begin(), pkt.payload.end());
                        
                        m_transport->Send(udpBuf.data(), udpBuf.size());
                    }
                }
            }
            m_capture->ReleaseFrame();
            frameCount++;
        } else {
            Sleep(1);
        }
    }
}

void StreamEngine::ReceiveLoop() {
    std::vector<uint8_t> rcvBuffer(65535);
    bool firstFrameDecoded = false;
    
    // NAL assembler simplificado (sem depacketizer rigoroso no Lote 8)
    std::vector<uint8_t> currentNal;
    
    while (m_running) {
        int bytes = m_transport->Receive(rcvBuffer.data(), rcvBuffer.size());
        if (bytes > sizeof(RtpHeader)) {
            RtpHeader* hdr = reinterpret_cast<RtpHeader*>(rcvBuffer.data());
            // Anexar payload
            currentNal.insert(currentNal.end(), rcvBuffer.begin() + sizeof(RtpHeader), rcvBuffer.begin() + bytes);
            
            if (hdr->marker) { // Fim da NAL
                if (m_decoder->DecodeNAL(currentNal)) {
                    if (!firstFrameDecoded) {
                        firstFrameDecoded = true;
                        // O cliente (Receiver) atingiu o estado Connected localmente
                        StreamHealthMonitor::Instance().SetStatus(HealthStep::Connected);
                        // Etapa 4: Descodificou, enviar ACK por TCP via SignalingClient para o Host saber
                        SignalingClient::Instance().SendStreamHealthStatus(m_targetIp, "VIDEO_VISIBLE");
                        // NOTA: Na vida real enviariamos um JSON STREAM_HEALTH. Mas p/ simplificação:
                    }
                }
                currentNal.clear();
            }
        } else if (bytes > 0 && bytes < sizeof(RtpHeader)) {
            std::string msg(rcvBuffer.begin(), rcvBuffer.begin() + bytes);
            if (msg == "PING") {
                StreamHealthMonitor::Instance().HookUdpPing(true);
                // Receiver also advances to PeerAck because UDP is arriving
                StreamHealthMonitor::Instance().SetStatus(HealthStep::PeerAck);
            }
        } else {
            Sleep(1);
        }
    }
}
