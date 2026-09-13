#pragma once
#include <vector>
#include <cstdint>
#include <memory>

class ReedSolomonFec {
public:
    ReedSolomonFec(int dataShards, int parityShards);
    ~ReedSolomonFec();

    static std::unique_ptr<ReedSolomonFec> CreateWithRedundancyPercent(int dataShards, int redundancyPercent = -1);

    void Encode(const std::vector<std::vector<uint8_t>>& dataShards, std::vector<std::vector<uint8_t>>& outParityShards);
    bool Decode(std::vector<std::vector<uint8_t>>& allShards, const std::vector<bool>& shardPresent);

private:
    int m_dataShards;
    int m_parityShards;
    int m_totalShards;
    std::vector<uint8_t> m_encodeMatrix;

    static uint8_t gf_mul(uint8_t a, uint8_t b);
    static uint8_t gf_div(uint8_t a, uint8_t b);
    void build_matrix();
    bool invert_matrix(std::vector<uint8_t>& mat, int size);
};
