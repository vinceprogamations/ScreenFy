#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <cstdint>

class SignalingClient {
public:
    static SignalingClient& Instance();

    bool Start(uint16_t port = 49152);
    void Stop();
    bool IsRunning() const { return m_running; }

    // Envio de comandos de sinalização
    bool SendCallRequest(const std::string& targetIp, const std::string& callerName, int videoPort);
    bool SendCallAccept(const std::string& targetIp);
    bool SendCallReject(const std::string& targetIp);
    bool SendCallEnd(const std::string& targetIp);
    bool SendStreamHealthStatus(const std::string& targetIp, const std::string& status);

    // Callbacks para UI
    std::function<void(const std::string& caller, const std::string& callerIp, int videoPort)> OnCallRequest;
    std::function<void(const std::string& targetIp)> OnCallAccepted;
    std::function<void(const std::string& targetIp)> OnCallRejected;
    std::function<void(const std::string& targetIp)> OnCallEnded;
    std::function<void(const std::string& targetIp, const std::string& status)> OnStreamHealthUpdate;

private:
    SignalingClient();
    ~SignalingClient();
    SignalingClient(const SignalingClient&) = delete;
    SignalingClient& operator=(const SignalingClient&) = delete;

    void ListenerWorker();
    void HeartbeatWorker();
    bool SendJson(const std::string& targetIp, uint16_t port, const std::string& jsonStr);

    std::atomic<bool> m_running{ false };
    uint16_t m_port = 49152;
    SOCKET m_listenSocket = INVALID_SOCKET;

    std::thread m_listenerThread;
    std::thread m_heartbeatThread;
};
