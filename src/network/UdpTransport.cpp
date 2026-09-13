#include "UdpTransport.h"
#include <iostream>

UdpTransport::UdpTransport() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
}

UdpTransport::~UdpTransport() {
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
    }
    WSACleanup();
}

bool UdpTransport::Initialize(const std::string& localIp, uint16_t localPort, const std::string& remoteIp, uint16_t remotePort) {
    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == INVALID_SOCKET) return false;

    int bufSize = 8 * 1024 * 1024;
    setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, (const char*)&bufSize, sizeof(bufSize));
    setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, (const char*)&bufSize, sizeof(bufSize));

    u_long nonBlocking = 1;
    ioctlsocket(m_socket, FIONBIO, &nonBlocking);

    sockaddr_in localAddr = {};
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(localPort);
    inet_pton(AF_INET, localIp.c_str(), &localAddr.sin_addr);

    if (bind(m_socket, (sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR) {
        return false;
    }

    m_remoteAddr.sin_family = AF_INET;
    m_remoteAddr.sin_port = htons(remotePort);
    inet_pton(AF_INET, remoteIp.c_str(), &m_remoteAddr.sin_addr);

    return true;
}

int UdpTransport::Send(const void* data, size_t size) {
    if (m_socket == INVALID_SOCKET) return -1;
    
    int result = sendto(m_socket, (const char*)data, static_cast<int>(size), 0,
                        (sockaddr*)&m_remoteAddr, sizeof(m_remoteAddr));
    
    if (result == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK) return 0;
        return -1;
    }
    return result;
}

int UdpTransport::Receive(void* buffer, size_t max_size) {
    if (m_socket == INVALID_SOCKET) return -1;

    sockaddr_in from = {};
    int fromLen = sizeof(from);
    
    int result = recvfrom(m_socket, (char*)buffer, static_cast<int>(max_size), 0,
                          (sockaddr*)&from, &fromLen);
                          
    if (result == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err == WSAEWOULDBLOCK) return 0;
        return -1;
    }
    return result;
}
