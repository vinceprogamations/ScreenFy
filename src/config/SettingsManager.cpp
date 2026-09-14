#include "SettingsManager.h"
#include "../identity/FriendManager.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <mmsystem.h>
#include <shlobj.h>

#pragma comment(lib, "winmm.lib")

using json = nlohmann::json;

SettingsManager& SettingsManager::Get() {
    static SettingsManager instance;
    return instance;
}

void SettingsManager::Initialize() {
    Load();
    // Sincronizar com o FriendManager
    if (!m_settings.Nickname.empty()) {
        FriendManager::Instance().SetMyNickname(m_settings.Nickname);
    }
    if (!m_settings.AvatarPath.empty()) {
        FriendManager::Instance().SetMyAvatarPath(m_settings.AvatarPath);
    }
    FriendManager::Instance().SetMyStatus(static_cast<UserStatus>(m_settings.Status));
}

std::string SettingsManager::GetConfigPath() const {
    char appData[MAX_PATH];
    if (SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, appData) == S_OK) {
        std::filesystem::path dir = std::filesystem::path(appData) / "ScreenShare4K";
        std::filesystem::create_directories(dir);
        return (dir / "config.json").string();
    }
    return "config.json";
}

AppSettings SettingsManager::GetSettings() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_settings;
}

void SettingsManager::UpdateSettings(const AppSettings& settings) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_settings = settings;
    }
    // Sincronizar dados de perfil caso tenham sido alterados
    FriendManager::Instance().SetMyNickname(settings.Nickname);
    FriendManager::Instance().SetMyAvatarPath(settings.AvatarPath);
    FriendManager::Instance().SetMyStatus(static_cast<UserStatus>(settings.Status));
    Save();
}

void SettingsManager::Save() {
    std::lock_guard<std::mutex> lock(m_mutex);
    try {
        json j;
        j["General"]["Nickname"] = m_settings.Nickname;
        j["General"]["AvatarPath"] = m_settings.AvatarPath;
        j["General"]["Status"] = m_settings.Status;

        j["Video"]["SelectedMonitorIndex"] = m_settings.SelectedMonitorIndex;
        j["Video"]["UseWGCFallback"] = m_settings.UseWGCFallback;
        j["Video"]["DefaultFPS"] = m_settings.DefaultFPS;
        j["Video"]["DefaultResolutionH"] = m_settings.DefaultResolutionH;

        j["Audio"]["SelectedMicrophone"] = m_settings.SelectedMicrophone;
        j["Audio"]["MicVolume"] = m_settings.MicVolume;
        j["Audio"]["AudioBitrateKbps"] = m_settings.AudioBitrateKbps;

        j["Network"]["TargetBitrateKbps"] = m_settings.TargetBitrateKbps;
        j["Network"]["FecRedundancyPercent"] = m_settings.FecRedundancyPercent;

        std::ofstream file(GetConfigPath());
        if (file.is_open()) {
            file << j.dump(4);
        }
    } catch (const std::exception& ex) {
        std::cerr << "[SettingsManager] Erro ao gravar configuracoes: " << ex.what() << std::endl;
    }
}

void SettingsManager::Load() {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string path = GetConfigPath();
    if (!std::filesystem::exists(path)) {
        // Se ainda não existir, salvar configuração padrão
        return;
    }

    try {
        std::ifstream file(path);
        if (!file.is_open()) return;

        json j;
        file >> j;

        if (j.contains("General") && j["General"].is_object()) {
            if (j["General"].contains("Nickname")) {
                m_settings.Nickname = j["General"]["Nickname"].get<std::string>();
            }
            if (j["General"].contains("AvatarPath")) {
                m_settings.AvatarPath = j["General"]["AvatarPath"].get<std::string>();
            }
            if (j["General"].contains("Status")) {
                m_settings.Status = j["General"]["Status"].get<int>();
            }
            FriendManager::Instance().SetMyNickname(m_settings.Nickname);
            FriendManager::Instance().SetMyAvatarPath(m_settings.AvatarPath);
            FriendManager::Instance().SetMyStatus(static_cast<UserStatus>(m_settings.Status));
        }

        if (j.contains("Video") && j["Video"].is_object()) {
            auto& v = j["Video"];
            if (v.contains("SelectedMonitorIndex")) m_settings.SelectedMonitorIndex = v["SelectedMonitorIndex"].get<int>();
            if (v.contains("UseWGCFallback")) m_settings.UseWGCFallback = v["UseWGCFallback"].get<bool>();
            if (v.contains("DefaultFPS")) m_settings.DefaultFPS = v["DefaultFPS"].get<int>();
            if (v.contains("DefaultResolutionH")) m_settings.DefaultResolutionH = v["DefaultResolutionH"].get<int>();
        }

        if (j.contains("Audio") && j["Audio"].is_object()) {
            auto& a = j["Audio"];
            if (a.contains("SelectedMicrophone")) m_settings.SelectedMicrophone = a["SelectedMicrophone"].get<std::string>();
            if (a.contains("MicVolume")) m_settings.MicVolume = a["MicVolume"].get<int>();
            if (a.contains("AudioBitrateKbps")) m_settings.AudioBitrateKbps = a["AudioBitrateKbps"].get<int>();
        }

        if (j.contains("Network") && j["Network"].is_object()) {
            auto& n = j["Network"];
            if (n.contains("TargetBitrateKbps")) m_settings.TargetBitrateKbps = n["TargetBitrateKbps"].get<int>();
            if (n.contains("FecRedundancyPercent")) m_settings.FecRedundancyPercent = n["FecRedundancyPercent"].get<int>();
        }
    } catch (const std::exception& ex) {
        std::cerr << "[SettingsManager] Erro ao carregar configuracoes: " << ex.what() << std::endl;
    }
}

namespace {
    struct MonitorEnumData {
        std::vector<std::string> monitors;
        int index = 0;
    };

    BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
        auto* data = reinterpret_cast<MonitorEnumData*>(dwData);
        MONITORINFOEXA mi;
        ZeroMemory(&mi, sizeof(mi));
        mi.cbSize = sizeof(MONITORINFOEXA);
        if (GetMonitorInfoA(hMonitor, &mi)) {
            int width = mi.rcMonitor.right - mi.rcMonitor.left;
            int height = mi.rcMonitor.bottom - mi.rcMonitor.top;
            std::string name = "Monitor " + std::to_string(data->index++) + " (" + 
                               std::string(mi.szDevice) + "): " + 
                               std::to_string(width) + "x" + std::to_string(height);
            if (mi.dwFlags & MONITORINFOF_PRIMARY) {
                name += " [Principal]";
            }
            data->monitors.push_back(name);
        }
        return TRUE;
    }
}

std::vector<std::string> SettingsManager::GetAvailableMonitors() {
    MonitorEnumData data;
    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&data));
    if (data.monitors.empty()) {
        data.monitors.push_back("Monitor 0: Display Principal (3840x2160) [Principal]");
    }
    return data.monitors;
}

std::vector<std::string> SettingsManager::GetAvailableMicrophones() {
    std::vector<std::string> mics;
    mics.push_back("Dispositivo Padrão do Sistema");

    UINT numDevs = waveInGetNumDevs();
    for (UINT i = 0; i < numDevs; ++i) {
        WAVEINCAPSA caps;
        ZeroMemory(&caps, sizeof(caps));
        if (waveInGetDevCapsA(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR) {
            mics.push_back(caps.szPname);
        }
    }
    return mics;
}
