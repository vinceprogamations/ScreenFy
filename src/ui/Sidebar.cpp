#include "Sidebar.h"
#include "ThemeDiscord.h"
#include "TextureLoader.h"
#include "../config/SettingsManager.h"
#include <imgui.h>
#include <iostream>
#include <algorithm>

Sidebar::Sidebar() = default;

std::optional<UserProfile> Sidebar::GetSelectedFriend() const {
    auto friends = FriendManager::Instance().GetFriends();
    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(friends.size())) {
        return friends[m_selectedIndex];
    }
    return std::nullopt;
}

void Sidebar::Render(float width, float height) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ThemeDiscord::COLOR_SIDEBAR_BG);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::BeginChild("SidebarRegion", ImVec2(width, height), false, ImGuiWindowFlags_NoScrollbar);

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // 1. Cabeçalho Brand Bar (Estilo Discord)
    ImVec2 headerStart = ImGui::GetCursorScreenPos();
    ImVec2 headerEnd = ImVec2(headerStart.x + width, headerStart.y + 64);
    dl->AddRectFilled(headerStart, headerEnd, IM_COL32(20, 21, 23, 255));
    dl->AddLine(ImVec2(headerStart.x, headerEnd.y), headerEnd, IM_COL32(35, 36, 40, 255), 1.0f);

    // Ícone do App "SF"
    ImVec2 logoPos = ImVec2(headerStart.x + 14, headerStart.y + 14);
    dl->AddRectFilled(logoPos, ImVec2(logoPos.x + 36, logoPos.y + 36), IM_COL32(88, 101, 242, 255), 10.0f);

    ImFont* fontTitle = ThemeDiscord::FontBold ? ThemeDiscord::FontBold : ThemeDiscord::FontRegular;
    if (fontTitle) {
        ImVec2 sfSize = fontTitle->CalcTextSizeA(16.0f, FLT_MAX, 0.0f, "SF");
        dl->AddText(fontTitle, 16.0f, ImVec2(logoPos.x + (36 - sfSize.x) * 0.5f, logoPos.y + (36 - sfSize.y) * 0.5f), IM_COL32(255, 255, 255, 255), "SF");
    }

    // Título e Subtítulo
    ImGui::SetCursorPos(ImVec2(58, 13));
    if (ThemeDiscord::FontHeader) ImGui::PushFont(ThemeDiscord::FontHeader);
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "ScreenFy");
    if (ThemeDiscord::FontHeader) ImGui::PopFont();

    ImGui::SetCursorPos(ImVec2(58, 35));
    if (ThemeDiscord::FontSmall) ImGui::PushFont(ThemeDiscord::FontSmall);
    ImGui::TextColored(ThemeDiscord::COLOR_BLURPLE, "DIRECT 4K STREAM");
    if (ThemeDiscord::FontSmall) ImGui::PopFont();

    ImGui::SetCursorPosY(74);

    // 2. Botão "Adicionar Amigo"
    float btnWidth = width - 24.0f;
    if (btnWidth < 180.0f) btnWidth = 180.0f;
    ImGui::SetCursorPosX(12);
    ImGui::PushStyleColor(ImGuiCol_Button, ThemeDiscord::COLOR_GREEN);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_GREEN_HOV);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeDiscord::COLOR_GREEN_ACT);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    if (ImGui::Button("+  Adicionar Amigo", ImVec2(btnWidth, 36))) {
        m_openAddModal = true;
        m_friendCodeBuffer[0] = '\0';
        m_addErrorMsg = "";
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    // 2.1 Campo de Busca Rápida (QOL)
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);
    ImGui::SetCursorPosX(12);
    ImGui::SetNextItemWidth(btnWidth);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(22, 23, 26, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
    ImGui::InputTextWithHint("##friendsearch", "🔍 Buscar amigo...", m_searchFilter, sizeof(m_searchFilter));
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);
    ImGui::SetCursorPosX(16);
    auto friends = FriendManager::Instance().GetFriends();
    if (ThemeDiscord::FontSmall) ImGui::PushFont(ThemeDiscord::FontSmall);
    ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "AMIGOS DIRETOS - %zu", friends.size());
    if (ThemeDiscord::FontSmall) ImGui::PopFont();

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 6);

    // 3. Lista de Amigos com Cartões Estilizados
    float listHeight = height - ImGui::GetCursorPosY() - 64.0f;
    if (listHeight < 100.0f) listHeight = 100.0f;
    ImGui::SetCursorPosX(10);
    ImGui::BeginChild("FriendsListScroll", ImVec2(width - 20.0f, listHeight), false, ImGuiWindowFlags_NoScrollbar);

    std::string query = m_searchFilter;
    std::transform(query.begin(), query.end(), query.begin(), ::tolower);

    for (int i = 0; i < static_cast<int>(friends.size()); i++) {
        const auto& f = friends[i];

        // Filtro de busca
        if (!query.empty()) {
            std::string nickLower = f.Nickname;
            std::transform(nickLower.begin(), nickLower.end(), nickLower.begin(), ::tolower);
            if (nickLower.find(query) == std::string::npos && f.RadminIP.find(query) == std::string::npos) {
                continue;
            }
        }

        ImGui::PushID(i);

        bool isSelected = (m_selectedIndex == i);
        ImVec2 itemStart = ImGui::GetCursorScreenPos();
        ImVec2 itemSize = ImVec2(width - 20.0f, 50);
        ImVec2 itemEnd = ImVec2(itemStart.x + itemSize.x, itemStart.y + itemSize.y);

        bool isHovered = ImGui::IsMouseHoveringRect(itemStart, itemEnd);

        if (ImGui::InvisibleButton("##friendBtn", itemSize)) {
            m_selectedIndex = i;
            if (OnFriendSelected) OnFriendSelected(f);
        }

        // Fundo do Cartão do Amigo
        ImDrawList* itemDl = ImGui::GetWindowDrawList();
        if (isSelected) {
            itemDl->AddRectFilled(itemStart, itemEnd, IM_COL32(53, 55, 60, 255), 6.0f);
            // Pílula vertical branca indicadora no canto esquerdo
            itemDl->AddRectFilled(ImVec2(itemStart.x + 2, itemStart.y + 10), ImVec2(itemStart.x + 6, itemStart.y + 40), IM_COL32(242, 243, 245, 255), 2.0f);
        } else if (isHovered) {
            itemDl->AddRectFilled(itemStart, itemEnd, IM_COL32(43, 45, 49, 200), 6.0f);
        }

        // Avatar do Amigo
        ID3D11ShaderResourceView* friendAvatarSrv = nullptr;
        if (!f.AvatarPath.empty()) {
            friendAvatarSrv = TextureLoader::Get().LoadTextureFromFile(f.AvatarPath);
        }
        ImVec2 avatarCenter = ImVec2(itemStart.x + 26, itemStart.y + 25);
        ThemeDiscord::DrawAvatar(itemDl, avatarCenter, 16.0f, f.Nickname, f.IsOnline, true, friendAvatarSrv);

        // Textos (Nickname + Status)
        ImVec2 nickPos = ImVec2(itemStart.x + 50, itemStart.y + 8);
        ImFont* fontNick = ThemeDiscord::FontBold ? ThemeDiscord::FontBold : ThemeDiscord::FontRegular;
        if (fontNick) {
            itemDl->AddText(fontNick, 15.0f, nickPos, IM_COL32(242, 243, 245, 255), f.Nickname.c_str());
        }

        ImVec2 statusPos = ImVec2(itemStart.x + 50, itemStart.y + 27);
        ImFont* fontStatus = ThemeDiscord::FontSmall ? ThemeDiscord::FontSmall : ThemeDiscord::FontRegular;
        if (fontStatus) {
            if (f.IsOnline) {
                std::string st = "Online • " + f.RadminIP;
                itemDl->AddText(fontStatus, 12.0f, statusPos, IM_COL32(35, 165, 90, 255), st.c_str());
            } else {
                std::string st = "Offline • " + f.RadminIP;
                itemDl->AddText(fontStatus, 12.0f, statusPos, IM_COL32(128, 132, 142, 255), st.c_str());
            }
        }

        // Context Menu para remover amigo
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Remover Contacto")) {
                FriendManager::Instance().RemoveFriend(f.RadminIP);
                if (m_selectedIndex == i) m_selectedIndex = -1;
                ImGui::EndPopup();
                ImGui::PopID();
                break;
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }

    ImGui::EndChild();

    // 4. Rodapé do Perfil do Utilizador Local
    float footerY = height - 60.0f;
    ImVec2 footerStart = ImVec2(headerStart.x, headerStart.y + footerY);
    ImVec2 footerEnd = ImVec2(footerStart.x + width, footerStart.y + 60);

    dl->AddRectFilled(footerStart, footerEnd, IM_COL32(17, 18, 20, 255));
    dl->AddLine(footerStart, ImVec2(footerStart.x + width, footerStart.y), IM_COL32(35, 36, 40, 255), 1.0f);

    auto myProf = FriendManager::Instance().GetMyProfile();
    ID3D11ShaderResourceView* myAvatarSrv = TextureLoader::Get().LoadTextureFromFile(myProf.AvatarPath);

    // Botão invisível sobre a área do avatar/nome para abrir menu rápido de perfil
    ImVec2 profBtnPos = footerStart;
    ImVec2 profBtnSize(width - 80.0f, 60.0f);
    ImGui::SetCursorPos(ImVec2(0, footerY));
    if (ImGui::InvisibleButton("##userProfileBtn", profBtnSize)) {
        ImGui::OpenPopup("QuickProfileMenu");
    }
    if (ImGui::IsItemHovered()) {
        dl->AddRectFilled(footerStart, ImVec2(footerStart.x + profBtnSize.x, footerStart.y + 60), IM_COL32(35, 37, 42, 120), 4.0f);
    }

    // Avatar do Utilizador
    ImVec2 myAvatarCenter = ImVec2(footerStart.x + 28, footerStart.y + 30);
    ThemeDiscord::DrawAvatar(dl, myAvatarCenter, 18.0f, myProf.Nickname, true, true, myAvatarSrv, static_cast<int>(myProf.Status));

    // Textos do Perfil
    ImFont* fontNick = ThemeDiscord::FontBold ? ThemeDiscord::FontBold : ThemeDiscord::FontRegular;
    if (fontNick) {
        dl->AddText(fontNick, 14.0f, ImVec2(footerStart.x + 54, footerStart.y + 12), IM_COL32(242, 243, 245, 255), myProf.Nickname.c_str());
    }

    ImFont* fontSub = ThemeDiscord::FontSmall ? ThemeDiscord::FontSmall : ThemeDiscord::FontRegular;
    if (fontSub) {
        const char* stName = "Online";
        switch (myProf.Status) {
            case UserStatus::Online: stName = "Online"; break;
            case UserStatus::Idle: stName = "Ausente"; break;
            case UserStatus::DND: stName = "Ocupado"; break;
            case UserStatus::Invisible: stName = "Invisível"; break;
        }
        std::string ipLabel = std::string(stName) + " • " + myProf.RadminIP;
        dl->AddText(fontSub, 12.0f, ImVec2(footerStart.x + 54, footerStart.y + 32), IM_COL32(148, 155, 164, 255), ipLabel.c_str());
    }

    // Menu Rápido de Status ao clicar no perfil
    if (ImGui::BeginPopup("QuickProfileMenu")) {
        if (ThemeDiscord::FontSmall) ImGui::PushFont(ThemeDiscord::FontSmall);
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "ALTERAR ESTADO:");
        if (ThemeDiscord::FontSmall) ImGui::PopFont();
        ImGui::Separator();

        if (ImGui::MenuItem("🟢  Online")) {
            auto s = SettingsManager::Get().GetSettings();
            s.Status = 0;
            SettingsManager::Get().UpdateSettings(s);
        }
        if (ImGui::MenuItem("🟡  Ausente")) {
            auto s = SettingsManager::Get().GetSettings();
            s.Status = 1;
            SettingsManager::Get().UpdateSettings(s);
        }
        if (ImGui::MenuItem("🔴  Não Incomodar")) {
            auto s = SettingsManager::Get().GetSettings();
            s.Status = 2;
            SettingsManager::Get().UpdateSettings(s);
        }
        if (ImGui::MenuItem("⚪  Invisível")) {
            auto s = SettingsManager::Get().GetSettings();
            s.Status = 3;
            SettingsManager::Get().UpdateSettings(s);
        }

        ImGui::Separator();
        if (ImGui::MenuItem("⚙  Editar Nome e Foto...")) {
            if (OnOpenSettings) OnOpenSettings();
        }

        ImGui::EndPopup();
    }

    // Botão de Tutorial [ ? ]
    ImGui::SetCursorPos(ImVec2(width - 76.0f, footerY + 14));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.25f, 0.28f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeDiscord::COLOR_BLURPLE);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 16.0f);
    if (ImGui::Button("?##HelpBtn", ImVec2(32, 32))) {
        if (OnOpenTutorial) {
            OnOpenTutorial();
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Como Usar / Tutorial para Amigos");
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    // Botão de Definições (Engrenagem estilizada)
    ImGui::SetCursorPos(ImVec2(width - 40.0f, footerY + 14));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.25f, 0.28f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeDiscord::COLOR_BLURPLE);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 16.0f);
    if (ImGui::Button("⚙##SettingsGear", ImVec2(34, 32))) {
        if (OnOpenSettings) {
            OnOpenSettings();
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Definicoes de Preferencias");
    }
    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);

    RenderAddFriendModal();

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

void Sidebar::RenderAddFriendModal() {
    if (m_openAddModal) {
        ImGui::OpenPopup("Adicionar Amigo##Modal");
        m_openAddModal = false;
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(480, 260));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24, 20));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ThemeDiscord::COLOR_CARD_BG);
    ImGui::PushStyleColor(ImGuiCol_Border, ThemeDiscord::COLOR_BORDER_SUBTLE);

    if (ImGui::BeginPopupModal("Adicionar Amigo##Modal", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        if (ThemeDiscord::FontHeader) ImGui::PushFont(ThemeDiscord::FontHeader);
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_PRIMARY, "Adicionar Amigo Direto");
        if (ThemeDiscord::FontHeader) ImGui::PopFont();

        ImGui::Spacing();
        ImGui::TextColored(ThemeDiscord::COLOR_TEXT_MUTED, "Cole abaixo o Friend Code Base64 partilhado pelo seu colega:");
        ImGui::Spacing();

        ImGui::SetNextItemWidth(432);
        ImGui::InputTextWithHint("##codeinput", "ex: Sm9obnwyNi4xNC41NS4xMDI=", m_friendCodeBuffer, sizeof(m_friendCodeBuffer));

        if (!m_addErrorMsg.empty()) {
            ImGui::Spacing();
            ImGui::TextColored(ThemeDiscord::COLOR_RED, "%s", m_addErrorMsg.c_str());
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 250);

        // Cancelar
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.26f, 0.28f, 0.5f));
        if (ImGui::Button("Cancelar", ImVec2(100, 36))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(2);

        ImGui::SameLine();

        // Adicionar
        ImGui::PushStyleColor(ImGuiCol_Button, ThemeDiscord::COLOR_BLURPLE);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ThemeDiscord::COLOR_BLURPLE_HOV);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ThemeDiscord::COLOR_BLURPLE_ACT);
        if (ImGui::Button("Adicionar", ImVec2(120, 36))) {
            std::string code(m_friendCodeBuffer);
            if (code.empty()) {
                m_addErrorMsg = "Por favor, insira o Friend Code.";
            } else if (FriendManager::Instance().AddFriendFromCode(code)) {
                ImGui::CloseCurrentPopup();
            } else {
                m_addErrorMsg = "Friend Code invalido ou formato incorreto.";
            }
        }
        ImGui::PopStyleColor(3);

        ImGui::EndPopup();
    }

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}
