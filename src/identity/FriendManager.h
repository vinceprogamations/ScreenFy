#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <optional>

enum class UserStatus {
    Online = 0,
    Idle = 1,
    DND = 2,
    Invisible = 3
};

struct UserProfile {
    std::string Nickname;
    std::string RadminIP;
    std::string AvatarPath;
    UserStatus Status = UserStatus::Online;
    bool IsOnline = false;
};

class FriendManager {
public:
    static FriendManager& Instance();

    void Initialize();

    // Gestão do Perfil Local
    UserProfile GetMyProfile() const;
    void SetMyNickname(const std::string& nickname);
    void SetMyRadminIP(const std::string& ip);
    void SetMyAvatarPath(const std::string& avatarPath);
    void SetMyStatus(UserStatus status);
    std::string GetMyFriendCode() const;

    // Utilitários de Friend Code (Base64)
    static std::string GenerateFriendCode(const std::string& nickname, const std::string& radminIp);
    static std::optional<UserProfile> ParseFriendCode(const std::string& base64Code);

    // Gestão de Amigos
    bool AddFriend(const UserProfile& profile);
    bool AddFriendFromCode(const std::string& base64Code);
    bool RemoveFriend(const std::string& radminIp);
    std::vector<UserProfile> GetFriends() const;
    void SetFriendOnline(const std::string& radminIp, bool isOnline);
    std::optional<UserProfile> FindFriend(const std::string& radminIp) const;

    // Persistência
    void Save();
    void Load();

    // Deteção de Rede
    static std::string DetectRadminIP();

private:
    FriendManager() = default;
    ~FriendManager() = default;
    FriendManager(const FriendManager&) = delete;
    FriendManager& operator=(const FriendManager&) = delete;

    std::string GetConfigPath() const;

    mutable std::mutex m_mutex;
    UserProfile m_myProfile;
    std::vector<UserProfile> m_friends;
};
