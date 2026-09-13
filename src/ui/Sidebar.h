#pragma once
#include <string>
#include <vector>
#include <functional>
#include "../identity/FriendManager.h"

class Sidebar {
public:
    Sidebar();
    ~Sidebar() = default;

    void Render(float width, float height);

    int GetSelectedIndex() const { return m_selectedIndex; }
    void SetSelectedIndex(int index) { m_selectedIndex = index; }
    std::optional<UserProfile> GetSelectedFriend() const;

    std::function<void(const UserProfile&)> OnFriendSelected;
    std::function<void()> OnOpenSettings;
    std::function<void()> OnOpenTutorial;

private:
    void RenderAddFriendModal();

    int m_selectedIndex = -1;
    bool m_openAddModal = false;
    char m_friendCodeBuffer[512] = "";
    char m_searchFilter[64] = "";
    std::string m_addErrorMsg = "";
};
