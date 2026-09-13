#include "../src/network/UdpTransport.h"
#include "../src/network/RtpPacketizer.h"
#include "../src/network/ReedSolomonFec.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <windows.h>

int main() {
    std::cout << "Starting Network Test (Lote 3)...\n";
    
    UdpTransport transport;
    if (!transport.Initialize("127.0.0.1", 5000, "127.0.0.1", 5000)) {
        std::cerr << "Falha ao iniciar UdpTransport.\n";
        return -1;
    }

    // Gerar um payload maciço simulando H.265 NAL Frame a 85Mbps de bitrate
    std::vector<uint8_t> syntheticNAL(15000); 
    for (size_t i = 0; i < syntheticNAL.size(); i++) {
        syntheticNAL[i] = static_cast<uint8_t>(i % 256);
    }

    RtpPacketizer packetizer(12345, 96);
    auto packets = packetizer.Packetize(syntheticNAL, 90000, true);
    
    std::cout << "RtpPacketizer partiu os 15KB em " << packets.size() << " fragmentos UDP/RTP.\n";

    // FEC Config: 10 Data, 2 Parity
    int dataShards = 10;
    int parityShards = 2;
    ReedSolomonFec fec(dataShards, parityShards);
    
    std::vector<std::vector<uint8_t>> fecBlock;
    for(int i = 0; i < std::min((int)packets.size(), dataShards); i++) {
        fecBlock.push_back(packets[i].payload);
    }
    while(fecBlock.size() < dataShards) fecBlock.push_back(std::vector<uint8_t>());

    std::vector<std::vector<uint8_t>> parity;
    fec.Encode(fecBlock, parity);
    std::cout << "FEC Gerou " << parity.size() << " blocos de paridade GF(2^8).\n";

    // Destruir os pacotes 1 e 3 (Pior cenário 20% perda no grupo)
    std::vector<bool> present(dataShards + parityShards, true);
    std::vector<std::vector<uint8_t>> receivedBlock = fecBlock;
    
    present[1] = false;
    present[3] = false;
    receivedBlock[1].clear();
    receivedBlock[3].clear();
    
    for(int i = 0; i < parityShards; i++) {
        receivedBlock.push_back(parity[i]);
    }

    std::cout << "A emular perda grave de rede (Pacotes 1 e 3 obliterados). A tentar curar...\n";

    if (fec.Decode(receivedBlock, present)) {
        std::cout << "FEC Decode executado com sucesso!\n";
        bool equal = (receivedBlock[1] == fecBlock[1]) && (receivedBlock[3] == fecBlock[3]);
        if (equal) {
            std::cout << "SUCESSO E ABSOLUTO: O conteudo recuperado pela matrix de Vandermonde GF(2^8) e 100% IDENTICO ao original!\n";
        } else {
            std::cerr << "Erro: Diferencas entre recuperado e original.\n";
            return -1;
        }
    } else {
        std::cerr << "Falha critica na descodificacao FEC.\n";
        return -1;
    }
    
    // Provar UDP Não-Bloqueante na porta local
    std::vector<uint8_t> serialPacket;
    serialPacket.insert(serialPacket.end(), (uint8_t*)&packets[0].header, (uint8_t*)&packets[0].header + sizeof(RtpHeader));
    serialPacket.insert(serialPacket.end(), packets[0].payload.begin(), packets[0].payload.end());

    transport.Send(serialPacket.data(), serialPacket.size());
    std::vector<uint8_t> rcvBuffer(2000);
    int recvd = 0;
    while ((recvd = transport.Receive(rcvBuffer.data(), rcvBuffer.size())) == 0) {
        Sleep(1);
    }
    std::cout << "UdpTransport nao-bloqueante roteou RTP (" << recvd << " bytes) pela socket local perfeitamente.\n";

    return 0;
}
