#include "FriendManager.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <shlobj.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

using json = nlohmann::json;

namespace {
    static const std::string BASE64_CHARS = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string Base64Encode(const std::string& input) {
        std::string ret;
        int val = 0, valb = -6;
        for (unsigned char c : input) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                ret.push_back(BASE64_CHARS[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) ret.push_back(BASE64_CHARS[((val << 8) >> (valb + 8)) & 0x3F]);
        while (ret.size() % 4) ret.push_back('=');
        return ret;
    }

    std::string Base64Decode(const std::string& input) {
        std::string ret;
        std::vector<int> T(256, -1);
        for (int i = 0; i < 64; i++) T[BASE64_CHARS[i]] = i;

        int val = 0, valb = -8;
        for (unsigned char c : input) {
            if (T[c] == -1) break;
            val = (val << 6) + T[c];
            valb += 6;
            if (valb >= 0) {
                ret.push_back(char((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        return ret;
    }
}

FriendManager& FriendManager::Instance() {
    static FriendManager instance;
    return instance;
}

void FriendManager::Initialize() {
    // Definir Nickname padrão se vazio
    char username[256] = "User";
    DWORD size = sizeof(username);
    if (GetUserNameA(username, &size)) {
        m_myProfile.Nickname = username;
    } else {
        m_myProfile.Nickname = "User4K";
    }

    // Tentar autodetectar o IP da Radmin VPN (faixa 26.x.x.x)
    m_myProfile.RadminIP = DetectRadminIP();
    m_myProfile.IsOnline = true;

    Load();
}

std::string FriendManager::DetectRadminIP() {
    ULONG outBufLen = 15000;
    std::vector<BYTE> buffer(outBufLen);
    PIP_ADAPTER_ADDRESSES pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());

    ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
    ULONG family = AF_INET; // Apenas IPv4

    DWORD dwRetVal = GetAdaptersAddresses(family, flags, nullptr, pAddresses, &outBufLen);
    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        buffer.resize(outBufLen);
        pAddresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
        dwRetVal = GetAdaptersAddresses(family, flags, nullptr, pAddresses, &outBufLen);
    }

    if (dwRetVal == NO_ERROR) {
        for (PIP_ADAPTER_ADDRESSES pCurr = pAddresses; pCurr != nullptr; pCurr = pCurr->Next) {
            for (PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurr->FirstUnicastAddress; pUnicast != nullptr; pUnicast = pUnicast->Next) {
                sockaddr_in* sa_in = reinterpret_cast<sockaddr_in*>(pUnicast->Address.lpSockaddr);
                char ipStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(sa_in->sin_addr), ipStr, INET_ADDRSTRLEN);
                std::string ip(ipStr);
                // Radmin VPN usa a gama 26.0.0.0/8
                if (ip.rfind("26.", 0) == 0) {
                    return ip;
                }
            }
        }
    }

    return "26.0.0.1"; // Valor predefinido caso Radmin VPN ainda não esteja ativa
}

std::string FriendManager::GetConfigPath() const {
    char appData[MAX_PATH];
    if (SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, appData) == S_OK) {
        std::filesystem::path dir = std::filesystem::path(appData) / "ScreenShare4K";
        std::filesystem::create_directories(dir);
        return (dir / "friends.json").string();
    }
    return "friends.json";
}

UserProfile FriendManager::GetMyProfile() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_myProfile;
}

void FriendManager::SetMyNickname(const std::string& nickname) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_myProfile.Nickname = nickname;
    }
    Save();
}

void FriendManager::SetMyRadminIP(const std::string& ip) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_myProfile.RadminIP = ip;
    }
    Save();
}

void FriendManager::SetMyAvatarPath(const std::string& avatarPath) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_myProfile.AvatarPath = avatarPath;
    }
    Save();
}

void FriendManager::SetMyStatus(UserStatus status) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_myProfile.Status = status;
    }
    Save();
}

std::string FriendManager::GetMyFriendCode() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return GenerateFriendCode(m_myProfile.Nickname, m_myProfile.RadminIP);
}

std::string FriendManager::GenerateFriendCode(const std::string& nickname, const std::string& radminIp) {
    std::string raw = nickname + "|" + radminIp;
    return Base64Encode(raw);
}

std::optional<UserProfile> FriendManager::ParseFriendCode(const std::string& base64Code) {
    std::string decoded = Base64Decode(base64Code);
    size_t sep = decoded.find('|');
    if (sep == std::string::npos || sep == 0 || sep == decoded.size() - 1) {
        return std::nullopt;
    }

    UserProfile profile;
    profile.Nickname = decoded.substr(0, sep);
    profile.RadminIP = decoded.substr(sep + 1);
    profile.IsOnline = false;
    return profile;
}

bool FriendManager::AddFriend(const UserProfile& profile) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& f : m_friends) {
        if (f.RadminIP == profile.RadminIP) {
            return false; // Já existe
        }
    }
    m_friends.push_back(profile);
    Save();
    return true;
}

bool FriendManager::AddFriendFromCode(const std::string& base64Code) {
    auto profile = ParseFriendCode(base64Code);
    if (!profile) return false;
    return AddFriend(*profile);
}

bool FriendManager::RemoveFriend(const std::string& radminIp) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::remove_if(m_friends.begin(), m_friends.end(), [&](const UserProfile& p) {
        return p.RadminIP == radminIp;
    });
    if (it != m_friends.end()) {
        m_friends.erase(it, m_friends.end());
        Save();
        return true;
    }
    return false;
}

std::vector<UserProfile> FriendManager::GetFriends() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_friends;
}

void FriendManager::SetFriendOnline(const std::string& radminIp, bool isOnline) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& f : m_friends) {
        if (f.RadminIP == radminIp) {
            f.IsOnline = isOnline;
            break;
        }
    }
}

std::optional<UserProfile> FriendManager::FindFriend(const std::string& radminIp) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& f : m_friends) {
        if (f.RadminIP == radminIp) return f;
    }
    return std::nullopt;
}

void FriendManager::Save() {
    std::string path = GetConfigPath();
    json j;
    j["myProfile"] = {
        {"nickname", m_myProfile.Nickname},
        {"radminIp", m_myProfile.RadminIP},
        {"avatarPath", m_myProfile.AvatarPath},
        {"status", static_cast<int>(m_myProfile.Status)}
    };

    json friendsArr = json::array();
    for (const auto& f : m_friends) {
        friendsArr.push_back({
            {"nickname", f.Nickname},
            {"radminIp", f.RadminIP},
            {"avatarPath", f.AvatarPath}
        });
    }
    j["friends"] = friendsArr;

    try {
        std::ofstream file(path);
        if (file.is_open()) {
            file << j.dump(4);
        }
    } catch (const std::exception& e) {
        std::cerr << "[FriendManager] Erro ao gravar " << path << ": " << e.what() << std::endl;
    }
}

void FriendManager::Load() {
    std::string path = GetConfigPath();
    if (!std::filesystem::exists(path)) return;

    try {
        std::ifstream file(path);
        if (!file.is_open()) return;

        json j;
        file >> j;

        if (j.contains("myProfile")) {
            auto p = j["myProfile"];
            if (p.contains("nickname")) m_myProfile.Nickname = p["nickname"].get<std::string>();
            if (p.contains("avatarPath")) m_myProfile.AvatarPath = p["avatarPath"].get<std::string>();
            if (p.contains("status")) m_myProfile.Status = static_cast<UserStatus>(p["status"].get<int>());
            if (p.contains("radminIp")) {
                std::string savedIp = p["radminIp"].get<std::string>();
                if (!savedIp.empty() && savedIp != "26.0.0.1") {
                    m_myProfile.RadminIP = savedIp;
                }
            }
        }

        if (j.contains("friends") && j["friends"].is_array()) {
            m_friends.clear();
            for (const auto& item : j["friends"]) {
                UserProfile f;
                f.Nickname = item.value("nickname", "Amigo");
                f.RadminIP = item.value("radminIp", "");
                f.AvatarPath = item.value("avatarPath", "");
                f.IsOnline = false;
                if (!f.RadminIP.empty()) {
                    m_friends.push_back(f);
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[FriendManager] Erro ao ler " << path << ": " << e.what() << std::endl;
    }
}
