#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <cstdint>

class UdpTransport {
public:
    UdpTransport();
    ~UdpTransport();

    bool Initialize(const std::string& localIp, uint16_t localPort, const std::string& remoteIp, uint16_t remotePort);
    
    int Send(const void* data, size_t size);
    int Receive(void* buffer, size_t max_size);

private:
    SOCKET m_socket = INVALID_SOCKET;
    sockaddr_in m_remoteAddr = {};
};
