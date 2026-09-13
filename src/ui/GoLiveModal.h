#pragma once
#include <string>
#include <vector>
#include <functional>
#include "../identity/FriendManager.h"

class GoLiveModal {
public:
    GoLiveModal();
    ~GoLiveModal() = default;

    // Abre o modal para o amigo especificado
    void Open(const UserProfile& targetFriend);

    // Renderiza o modal se estiver aberto
    void Render();

    bool IsOpen() const { return m_isOpen; }

    // Callback ao confirmar a transmissão
    std::function<void(const UserProfile& target, int resolutionH, int fps, int monitorIdx)> OnConfirm;

private:
    bool m_isOpen = false;
    UserProfile m_targetFriend;

    // Seleção de aba: 0 = Telas, 1 = Aplicativos
    int m_selectedTab = 0;

    // Opções de Stream
    int m_selectedMonitorIdx = 0;
    int m_selectedResIdx = 1;  // 0: 720p, 1: 1080p, 2: 1440p, 3: 4K
    int m_selectedFpsIdx = 1;  // 0: 30, 1: 60, 2: 120
    int m_selectedAudioBitrateIdx = 1; // 0: 64, 1: 128, 2: 192
};
