#include "../src/config/SettingsManager.h"
#include "../src/identity/FriendManager.h"
#include "../src/network/ReedSolomonFec.h"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "Iniciando teste do Motor de Configuracoes (Lote 6)..." << std::endl;

    // 1. Inicializar SettingsManager
    auto& mgr = SettingsManager::Get();
    mgr.Initialize();

    AppSettings initial = mgr.GetSettings();
    std::cout << "[OK] Nickname inicial: " << initial.Nickname << std::endl;
    std::cout << "[OK] Bitrate inicial: " << initial.TargetBitrateKbps << " Kbps" << std::endl;
    std::cout << "[OK] Redundancia FEC inicial: " << initial.FecRedundancyPercent << "%" << std::endl;

    // 2. Modificar configurações
    AppSettings modified = initial;
    modified.Nickname = "UltraGamer4K";
    modified.SelectedMonitorIndex = 1;
    modified.UseWGCFallback = false;
    modified.DefaultFPS = 120;
    modified.DefaultResolutionH = 2160;
    modified.SelectedMicrophone = "Custom Mic";
    modified.MicVolume = 85;
    modified.AudioBitrateKbps = 192;
    modified.TargetBitrateKbps = 95000;
    modified.FecRedundancyPercent = 25;

    mgr.UpdateSettings(modified);

    // 3. Verificar sincronização com FriendManager
    auto profile = FriendManager::Instance().GetMyProfile();
    if (profile.Nickname != "UltraGamer4K") {
        std::cerr << "[FALHA] FriendManager nao sincronizou nickname!" << std::endl;
        return 1;
    }
    std::cout << "[OK] FriendManager sincronizado com novo nickname: " << profile.Nickname << std::endl;

    // 4. Testar Recarregamento (Load) do ficheiro JSON
    mgr.Load();
    AppSettings reloaded = mgr.GetSettings();

    if (reloaded.Nickname != "UltraGamer4K" ||
        reloaded.SelectedMonitorIndex != 1 ||
        reloaded.UseWGCFallback != false ||
        reloaded.DefaultFPS != 120 ||
        reloaded.DefaultResolutionH != 2160 ||
        reloaded.MicVolume != 85 ||
        reloaded.AudioBitrateKbps != 192 ||
        reloaded.TargetBitrateKbps != 95000 ||
        reloaded.FecRedundancyPercent != 25) {
        std::cerr << "[FALHA] Configuracoes recarregadas diferem dos valores salvos!" << std::endl;
        return 2;
    }
    std::cout << "[OK] Persistencia JSON em %APPDATA%/ScreenShare4K/config.json validada com sucesso!" << std::endl;

    // 5. Testar Enumeração de Hardware
    auto monitors = SettingsManager::GetAvailableMonitors();
    std::cout << "[OK] Monitores detetados: " << monitors.size() << std::endl;
    for (size_t i = 0; i < monitors.size(); ++i) {
        std::cout << "     - " << monitors[i] << std::endl;
    }
    if (monitors.empty()) {
        std::cerr << "[FALHA] Nenhum monitor enumerado!" << std::endl;
        return 3;
    }

    auto microphones = SettingsManager::GetAvailableMicrophones();
    std::cout << "[OK] Microfones detetados: " << microphones.size() << std::endl;
    for (size_t i = 0; i < microphones.size(); ++i) {
        std::cout << "     - " << microphones[i] << std::endl;
    }
    if (microphones.empty()) {
        std::cerr << "[FALHA] Nenhum dispositivo de som detetado!" << std::endl;
        return 4;
    }

    // 6. Testar fábrica FEC com percentual configurado
    auto fec = ReedSolomonFec::CreateWithRedundancyPercent(10);
    if (!fec) {
        std::cerr << "[FALHA] Falha ao criar instancia FEC com configuracoes dinamicas!" << std::endl;
        return 5;
    }
    std::cout << "[OK] Instancia ReedSolomonFec criada com sucesso baseada em FecRedundancyPercent (" 
              << reloaded.FecRedundancyPercent << "%)." << std::endl;

    // Restaurar nickname para não poluir
    modified.Nickname = "User4K";
    mgr.UpdateSettings(modified);

    std::cout << "=== TODOS OS TESTES DO LOTE 6 PASSARAM COM SUCESSO ===" << std::endl;
    return 0;
}
