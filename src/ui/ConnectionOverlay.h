#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <string>
#include <functional>

class ConnectionOverlay {
public:
    ConnectionOverlay();
    
    void Render(float screenWidth, float screenHeight);
    
    void StartConnection(const std::string& peerName, const std::string& peerIp, bool isHost);
    void CancelConnection();
    
    bool IsActive() const { return m_isActive; }

    std::function<void()> OnConnectionEstablished;
    std::function<void()> OnConnectionCancelled;

private:
    bool m_isActive = false;
    bool m_isHost = false;
    std::string m_peerName;
    std::string m_peerIp;
    
public:
    std::string GetPeerName() const { return m_peerName; }
    std::string GetPeerIp() const { return m_peerIp; }
};
