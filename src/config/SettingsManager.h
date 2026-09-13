#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <string>
#include <vector>
#include <mutex>

struct AppSettings {
    // Geral
    std::string Nickname = "User4K";
    std::string AvatarPath = "";
    int Status = 0; // 0=Online, 1=Idle, 2=DND, 3=Invisible

    // Vídeo
    int SelectedMonitorIndex = 0;
    bool UseWGCFallback = true;
    int DefaultFPS = 60;
    int DefaultResolutionH = 1080;

    // Áudio
    std::string SelectedMicrophone = "Default";
    int MicVolume = 100;
    int AudioBitrateKbps = 128;

    // Rede
    int TargetBitrateKbps = 85000;
    int FecRedundancyPercent = 20;
};

class SettingsManager {
public:
    static SettingsManager& Get();

    void Initialize();
    AppSettings GetSettings() const;
    void UpdateSettings(const AppSettings& settings);
    void Save();
    void Load();

    // Utilitários de Enumeração de Hardware
    static std::vector<std::string> GetAvailableMonitors();
    static std::vector<std::string> GetAvailableMicrophones();

private:
    SettingsManager() = default;
    ~SettingsManager() = default;
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    std::string GetConfigPath() const;

    mutable std::mutex m_mutex;
    AppSettings m_settings;
};
