#include "ReedSolomonFec.h"
#include "../config/SettingsManager.h"
#include <stdexcept>
#include <algorithm>

std::unique_ptr<ReedSolomonFec> ReedSolomonFec::CreateWithRedundancyPercent(int dataShards, int redundancyPercent) {
    if (redundancyPercent < 0) {
        redundancyPercent = SettingsManager::Get().GetSettings().FecRedundancyPercent;
    }
    int parityShards = (dataShards * redundancyPercent + 99) / 100;
    if (parityShards < 1) parityShards = 1;
    return std::make_unique<ReedSolomonFec>(dataShards, parityShards);
}

static uint8_t gf_exp[512];
static uint8_t gf_log[256];
static bool gf_init = false;

static void init_gf() {
    if (gf_init) return;
    int x = 1;
    for (int i = 0; i < 255; i++) {
        gf_exp[i] = static_cast<uint8_t>(x);
        gf_exp[i + 255] = static_cast<uint8_t>(x);
        gf_log[x] = static_cast<uint8_t>(i);
        x <<= 1;
        if (x & 0x100) x ^= 0x11D; // Polinómio AES/Rijndael
    }
    gf_log[0] = 0;
    gf_init = true;
}

uint8_t ReedSolomonFec::gf_mul(uint8_t a, uint8_t b) {
    if (a == 0 || b == 0) return 0;
    return gf_exp[gf_log[a] + gf_log[b]];
}

uint8_t ReedSolomonFec::gf_div(uint8_t a, uint8_t b) {
    if (a == 0) return 0;
    if (b == 0) throw std::runtime_error("Divide by zero GF(2^8)");
    return gf_exp[(gf_log[a] + 255 - gf_log[b]) % 255];
}

ReedSolomonFec::ReedSolomonFec(int dataShards, int parityShards) 
    : m_dataShards(dataShards), m_parityShards(parityShards), m_totalShards(dataShards + parityShards) {
    init_gf();
    build_matrix();
}

ReedSolomonFec::~ReedSolomonFec() = default;

void ReedSolomonFec::build_matrix() {
    // Matriz de Vandermonde M x N para a paridade
    m_encodeMatrix.resize(m_parityShards * m_dataShards);
    for (int r = 0; r < m_parityShards; r++) {
        for (int c = 0; c < m_dataShards; c++) {
            // Um gerador simples de polinómio
            uint8_t val = gf_exp[(r * c) % 255];
            m_encodeMatrix[r * m_dataShards + c] = val;
        }
    }
}

void ReedSolomonFec::Encode(const std::vector<std::vector<uint8_t>>& dataShards, std::vector<std::vector<uint8_t>>& outParityShards) {
    size_t shardSize = 0;
    for (auto& s : dataShards) {
        if (s.size() > shardSize) shardSize = s.size();
    }

    outParityShards.assign(m_parityShards, std::vector<uint8_t>(shardSize, 0));

    for (int r = 0; r < m_parityShards; r++) {
        for (int c = 0; c < m_dataShards; c++) {
            uint8_t coeff = m_encodeMatrix[r * m_dataShards + c];
            if (coeff == 0 || c >= dataShards.size() || dataShards[c].empty()) continue;
            
            for (size_t i = 0; i < dataShards[c].size(); i++) {
                outParityShards[r][i] ^= gf_mul(dataShards[c][i], coeff);
            }
        }
    }
}

bool ReedSolomonFec::invert_matrix(std::vector<uint8_t>& mat, int size) {
    for (int r = 0; r < size; r++) {
        int pivot = r;
        while (pivot < size && mat[pivot * size + r] == 0) pivot++;
        if (pivot == size) return false;
        
        if (pivot != r) {
            for (int c = 0; c < size; c++) std::swap(mat[r * size + c], mat[pivot * size + c]);
        }
        
        uint8_t scale = mat[r * size + r];
        for (int c = 0; c < size; c++) mat[r * size + c] = gf_div(mat[r * size + c], scale);
        
        for (int row = 0; row < size; row++) {
            if (row != r && mat[row * size + r] != 0) {
                uint8_t factor = mat[row * size + r];
                for (int c = 0; c < size; c++) {
                    mat[row * size + c] ^= gf_mul(mat[r * size + c], factor);
                }
            }
        }
    }
    return true;
}

bool ReedSolomonFec::Decode(std::vector<std::vector<uint8_t>>& allShards, const std::vector<bool>& shardPresent) {
    int missingCount = 0;
    std::vector<int> missingIndices;
    std::vector<int> presentIndices;

    for (int i = 0; i < m_dataShards; i++) {
        if (!shardPresent[i]) {
            missingCount++;
            missingIndices.push_back(i);
        } else {
            presentIndices.push_back(i);
        }
    }
    if (missingCount == 0) return true;
    if (missingCount > m_parityShards) return false; // Perda irrecuperável

    for (int i = m_dataShards; i < m_totalShards && presentIndices.size() < m_dataShards; i++) {
        if (shardPresent[i]) presentIndices.push_back(i);
    }
    
    if (presentIndices.size() < m_dataShards) return false;

    // Criar matriz de decodificação
    std::vector<uint8_t> decodeMat(m_dataShards * m_dataShards, 0);
    for (int r = 0; r < m_dataShards; r++) {
        int srcRow = presentIndices[r];
        if (srcRow < m_dataShards) {
            decodeMat[r * m_dataShards + srcRow] = 1; // Matriz Identidade para dados
        } else {
            int pRow = srcRow - m_dataShards;
            for (int c = 0; c < m_dataShards; c++) {
                decodeMat[r * m_dataShards + c] = m_encodeMatrix[pRow * m_dataShards + c];
            }
        }
    }

    // Inverter usando um sistema estendido
    std::vector<uint8_t> invMat(m_dataShards * m_dataShards * 2, 0);
    for (int r = 0; r < m_dataShards; r++) {
        for (int c = 0; c < m_dataShards; c++) {
            invMat[r * (m_dataShards * 2) + c] = decodeMat[r * m_dataShards + c];
        }
        invMat[r * (m_dataShards * 2) + m_dataShards + r] = 1;
    }

    // Gaussian Elimination
    for (int r = 0; r < m_dataShards; r++) {
        int pivot = r;
        while (pivot < m_dataShards && invMat[pivot * (m_dataShards * 2) + r] == 0) pivot++;
        if (pivot == m_dataShards) return false;
        
        if (pivot != r) {
            for (int c = 0; c < m_dataShards * 2; c++) std::swap(invMat[r * (m_dataShards * 2) + c], invMat[pivot * (m_dataShards * 2) + c]);
        }
        
        uint8_t scale = invMat[r * (m_dataShards * 2) + r];
        for (int c = 0; c < m_dataShards * 2; c++) invMat[r * (m_dataShards * 2) + c] = gf_div(invMat[r * (m_dataShards * 2) + c], scale);
        
        for (int row = 0; row < m_dataShards; row++) {
            if (row != r && invMat[row * (m_dataShards * 2) + r] != 0) {
                uint8_t factor = invMat[row * (m_dataShards * 2) + r];
                for (int c = 0; c < m_dataShards * 2; c++) {
                    invMat[row * (m_dataShards * 2) + c] ^= gf_mul(invMat[r * (m_dataShards * 2) + c], factor);
                }
            }
        }
    }

    size_t shardSize = 0;
    for (int r : presentIndices) {
        if (allShards[r].size() > shardSize) shardSize = allShards[r].size();
    }

    std::vector<std::vector<uint8_t>> decoded(missingCount, std::vector<uint8_t>(shardSize, 0));
    
    for (int i = 0; i < missingCount; i++) {
        int mIdx = missingIndices[i];
        for (int r = 0; r < m_dataShards; r++) {
            uint8_t coeff = invMat[mIdx * (m_dataShards * 2) + m_dataShards + r];
            if (coeff == 0) continue;
            int srcIdx = presentIndices[r];
            for (size_t b = 0; b < shardSize; b++) {
                uint8_t val = b < allShards[srcIdx].size() ? allShards[srcIdx][b] : 0;
                decoded[i][b] ^= gf_mul(val, coeff);
            }
        }
        allShards[mIdx] = std::move(decoded[i]);
    }

    return true;
}
