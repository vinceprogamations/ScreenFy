#pragma once
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include "../identity/FriendManager.h"

#include "GoLiveModal.h"
#include "ScreenTestModal.h"

class ChatPanel {
public:
    ChatPanel();
    ~ChatPanel() = default;

    void Render(const std::optional<UserProfile>& selectedFriend, float width, float height);
    void AddLog(const std::string& message);

    int GetSelectedResolution() const { return m_selectedResolution; }
    int GetSelectedFps() const { return m_selectedFps; }

    std::function<void(const UserProfile& target, int resolution, int fps)> OnStartCall;
    std::function<void()> OnOpenTutorial;

private:
    GoLiveModal m_goLiveModal;
    ScreenTestModal m_screenTestModal;
    std::optional<UserProfile> m_currentFriend;

    int m_selectedResolution = 0; // 0: 4K, 1: 1440p, 2: 1080p
    int m_selectedFps = 0;        // 0: 120 FPS, 1: 60 FPS

    bool m_copiedNotification = false;
    float m_copiedTimer = 0.0f;

    std::vector<std::string> m_logs;
};
