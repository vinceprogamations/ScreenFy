#include "SignalingClient.h"
#include "../identity/FriendManager.h"
#include <nlohmann/json.hpp>
#include <ws2tcpip.h>
#include <iostream>
#include <chrono>

using json = nlohmann::json;

SignalingClient& SignalingClient::Instance() {
    static SignalingClient instance;
    return instance;
}

SignalingClient::SignalingClient() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
}

SignalingClient::~SignalingClient() {
    Stop();
    WSACleanup();
}

bool SignalingClient::Start(uint16_t port) {
    if (m_running) return true;
    m_port = port;

    m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listenSocket == INVALID_SOCKET) {
        std::cerr << "[SignalingClient] Erro ao criar listen socket: " << WSAGetLastError() << std::endl;
        return false;
    }

    BOOL opt = TRUE;
    setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&opt), sizeof(opt));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(m_port);

    if (bind(m_listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[SignalingClient] Falha ao fazer bind na porta " << m_port << ": " << WSAGetLastError() << std::endl;
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
        return false;
    }

    if (listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "[SignalingClient] Falha ao escutar na porta: " << WSAGetLastError() << std::endl;
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
        return false;
    }

    m_running = true;
    m_listenerThread = std::thread(&SignalingClient::ListenerWorker, this);
    m_heartbeatThread = std::thread(&SignalingClient::HeartbeatWorker, this);

    std::cout << "[SignalingClient] Servidor de sinalização ativo na porta " << m_port << std::endl;
    return true;
}

void SignalingClient::Stop() {
    if (!m_running) return;
    m_running = false;

    if (m_listenSocket != INVALID_SOCKET) {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }

    if (m_listenerThread.joinable()) m_listenerThread.join();
    if (m_heartbeatThread.joinable()) m_heartbeatThread.join();
}

void SignalingClient::ListenerWorker() {
    while (m_running) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(m_listenSocket, &readSet);

        timeval tv{ 1, 0 }; // 1s timeout
        int sel = select(0, &readSet, nullptr, nullptr, &tv);
        if (sel <= 0 || !m_running) continue;

        sockaddr_in clientAddr{};
        int clientLen = sizeof(clientAddr);
        SOCKET clientSock = accept(m_listenSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (clientSock == INVALID_SOCKET) continue;

        char clientIpStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIpStr, INET_ADDRSTRLEN);
        std::string clientIp(clientIpStr);

        // Receber payload JSON
        char buffer[4096];
        int bytesRecv = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
        if (bytesRecv > 0) {
            buffer[bytesRecv] = '\0';
            try {
                auto j = json::parse(buffer);
                std::string type = j.value("type", "");

                if (type == "PING") {
                    std::string pong = "{\"type\":\"PONG\"}";
                    send(clientSock, pong.c_str(), static_cast<int>(pong.size()), 0);
                } else if (type == "CALL_REQUEST") {
                    std::string caller = j.value("caller", "Desconhecido");
                    int vPort = j.value("video_port", 50000);
                    if (OnCallRequest) {
                        OnCallRequest(caller, clientIp, vPort);
                    }
                } else if (type == "CALL_ACCEPT") {
                    if (OnCallAccepted) OnCallAccepted(clientIp);
                } else if (type == "CALL_REJECT") {
                    if (OnCallRejected) OnCallRejected(clientIp);
                } else if (type == "CALL_END") {
                    if (OnCallEnded) OnCallEnded(clientIp);
                }
            } catch (const std::exception& e) {
                std::cerr << "[SignalingClient] Erro ao processar mensagem JSON: " << e.what() << std::endl;
            }
        }
        closesocket(clientSock);
    }
}

void SignalingClient::HeartbeatWorker() {
    while (m_running) {
        auto friends = FriendManager::Instance().GetFriends();
        for (const auto& f : friends) {
            if (!m_running) break;
            if (f.RadminIP.empty()) continue;

            // Testar connect não-bloqueante na porta 49152
            SOCKET testSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (testSock == INVALID_SOCKET) continue;

            u_long mode = 1; // Não-bloqueante
            ioctlsocket(testSock, FIONBIO, &mode);

            sockaddr_in targetAddr{};
            targetAddr.sin_family = AF_INET;
            targetAddr.sin_port = htons(m_port);
            inet_pton(AF_INET, f.RadminIP.c_str(), &targetAddr.sin_addr);

            connect(testSock, reinterpret_cast<sockaddr*>(&targetAddr), sizeof(targetAddr));

            fd_set writeSet;
            FD_ZERO(&writeSet);
            FD_SET(testSock, &writeSet);

            timeval tv{ 0, 400000 }; // 400ms timeout
            int res = select(0, nullptr, &writeSet, nullptr, &tv);

            bool isOnline = false;
            if (res > 0) {
                int err = 0;
                int len = sizeof(err);
                getsockopt(testSock, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&err), &len);
                if (err == 0) {
                    isOnline = true;
                }
            }

            FriendManager::Instance().SetFriendOnline(f.RadminIP, isOnline);
            closesocket(testSock);
        }

        // Aguardar 5 segundos entre cada rodada de heartbeats
        for (int i = 0; i < 50 && m_running; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

bool SignalingClient::SendJson(const std::string& targetIp, uint16_t port, const std::string& jsonStr) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return false;

    sockaddr_in targetAddr{};
    targetAddr.sin_family = AF_INET;
    targetAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, targetIp.c_str(), &targetAddr.sin_addr) <= 0) {
        closesocket(sock);
        return false;
    }

    // Timeout de envio/receção de 2 segundos
    DWORD timeout = 2000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<char*>(&timeout), sizeof(timeout));

    if (connect(sock, reinterpret_cast<sockaddr*>(&targetAddr), sizeof(targetAddr)) == SOCKET_ERROR) {
        closesocket(sock);
        return false;
    }

    int sent = send(sock, jsonStr.c_str(), static_cast<int>(jsonStr.size()), 0);
    closesocket(sock);
    return sent > 0;
}

bool SignalingClient::SendCallRequest(const std::string& targetIp, const std::string& callerName, int videoPort) {
    json j = {
        {"type", "CALL_REQUEST"},
        {"caller", callerName},
        {"video_port", videoPort}
    };
    return SendJson(targetIp, m_port, j.dump());
}

bool SignalingClient::SendCallAccept(const std::string& targetIp) {
    json j = { {"type", "CALL_ACCEPT"} };
    return SendJson(targetIp, m_port, j.dump());
}

bool SignalingClient::SendCallReject(const std::string& targetIp) {
    json j = { {"type", "CALL_REJECT"} };
    return SendJson(targetIp, m_port, j.dump());
}

bool SignalingClient::SendCallEnd(const std::string& targetIp) {
    json j = { {"type", "CALL_END"} };
    return SendJson(targetIp, m_port, j.dump());
}
