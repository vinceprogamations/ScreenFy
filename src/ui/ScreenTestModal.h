#pragma once
#include <string>
#include <functional>

class ScreenTestModal {
public:
    ScreenTestModal();
    ~ScreenTestModal() = default;

    void Open();
    void Close();
    void Render();

    bool IsOpen() const { return m_isOpen; }

    // Callback para abrir o GoLiveModal diretamente do estúdio de teste
    std::function<void()> OnGoLiveRequested;

private:
    bool m_isOpen = false;
};
