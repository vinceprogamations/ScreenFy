#pragma once
#include <imgui.h>
#include <filesystem>
#include <string>
#include <algorithm>

namespace ThemeDiscord {
    // Paleta de Cores Premium Discord (Darker Theme)
    inline const ImVec4 COLOR_WINDOW_BG    = ImVec4(0.122f, 0.125f, 0.133f, 1.0f); // #1E1F22
    inline const ImVec4 COLOR_SIDEBAR_BG   = ImVec4(0.106f, 0.110f, 0.118f, 1.0f); // #1B1C1E
    inline const ImVec4 COLOR_POPUP_BG     = ImVec4(0.118f, 0.122f, 0.133f, 1.0f); // #1E1F22
    inline const ImVec4 COLOR_HEADER_BG    = ImVec4(0.141f, 0.145f, 0.157f, 1.0f); // #242528

    inline const ImVec4 COLOR_CARD_BG      = ImVec4(0.169f, 0.176f, 0.192f, 1.0f); // #2B2D31
    inline const ImVec4 COLOR_CARD_HOVER   = ImVec4(0.208f, 0.216f, 0.235f, 1.0f); // #35373C
    inline const ImVec4 COLOR_CARD_ACTIVE  = ImVec4(0.231f, 0.243f, 0.267f, 1.0f); // #3B3E44
    inline const ImVec4 COLOR_INPUT_BG     = ImVec4(0.086f, 0.090f, 0.098f, 1.0f); // #161719

    inline const ImVec4 COLOR_BLURPLE      = ImVec4(0.345f, 0.396f, 0.949f, 1.0f); // #5865F2
    inline const ImVec4 COLOR_BLURPLE_HOV  = ImVec4(0.278f, 0.322f, 0.769f, 1.0f); // #4752C4
    inline const ImVec4 COLOR_BLURPLE_ACT  = ImVec4(0.235f, 0.271f, 0.647f, 1.0f); // #3C45A5

    inline const ImVec4 COLOR_GREEN        = ImVec4(0.137f, 0.647f, 0.353f, 1.0f); // #23A55A
    inline const ImVec4 COLOR_GREEN_HOV    = ImVec4(0.122f, 0.573f, 0.310f, 1.0f); // #1F924F
    inline const ImVec4 COLOR_GREEN_ACT    = ImVec4(0.102f, 0.482f, 0.259f, 1.0f); // #1A7B42

    inline const ImVec4 COLOR_RED          = ImVec4(0.855f, 0.216f, 0.235f, 1.0f); // #DA373C
    inline const ImVec4 COLOR_RED_HOV      = ImVec4(0.706f, 0.157f, 0.176f, 1.0f); // #B4282D
    inline const ImVec4 COLOR_RED_ACT      = ImVec4(0.561f, 0.125f, 0.137f, 1.0f); // #8F2023

    inline const ImVec4 COLOR_GRAY_MUTED   = ImVec4(0.502f, 0.518f, 0.557f, 1.0f); // #80848E
    inline const ImVec4 COLOR_BORDER_SUBTLE= ImVec4(0.220f, 0.227f, 0.247f, 0.6f); // #383A3F
    inline const ImVec4 COLOR_TEXT_PRIMARY = ImVec4(0.961f, 0.965f, 0.973f, 1.0f); // #F5F6F8
    inline const ImVec4 COLOR_TEXT_MUTED   = ImVec4(0.580f, 0.608f, 0.643f, 1.0f); // #949BA4
    inline const ImVec4 COLOR_TEXT_SUBTLE  = ImVec4(0.427f, 0.451f, 0.494f, 1.0f); // #6D737E

    // Fontes
    inline ImFont* FontRegular = nullptr;
    inline ImFont* FontBold    = nullptr;
    inline ImFont* FontHeader  = nullptr;
    inline ImFont* FontTitle   = nullptr;
    inline ImFont* FontSmall   = nullptr;

    inline void ApplyStyle() {
        ImGuiStyle& style = ImGui::GetStyle();

        // Rounding Suave Moderno
        style.WindowRounding    = 8.0f;
        style.ChildRounding     = 8.0f;
        style.FrameRounding     = 6.0f;
        style.PopupRounding     = 8.0f;
        style.ScrollbarRounding = 6.0f;
        style.GrabRounding      = 6.0f;
        style.TabRounding       = 6.0f;

        // Bordas Sutis e Elegantes
        style.WindowBorderSize  = 0.0f;
        style.ChildBorderSize   = 1.0f;
        style.PopupBorderSize   = 1.0f;
        style.FrameBorderSize   = 1.0f;

        // Espaçamento e Respiração Visual
        style.WindowPadding     = ImVec2(14, 14);
        style.FramePadding      = ImVec2(12, 8);
        style.ItemSpacing       = ImVec2(10, 10);
        style.ItemInnerSpacing  = ImVec2(8, 6);
        style.ScrollbarSize     = 10.0f;

        // Mapeamento de Cores ImGui
        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text]                  = COLOR_TEXT_PRIMARY;
        colors[ImGuiCol_TextDisabled]          = COLOR_TEXT_MUTED;
        colors[ImGuiCol_WindowBg]              = COLOR_WINDOW_BG;
        colors[ImGuiCol_ChildBg]               = COLOR_SIDEBAR_BG;
        colors[ImGuiCol_PopupBg]               = COLOR_POPUP_BG;
        colors[ImGuiCol_Border]                = COLOR_BORDER_SUBTLE;
        colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.0f);

        colors[ImGuiCol_FrameBg]               = COLOR_INPUT_BG;
        colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.12f, 0.13f, 0.14f, 1.0f);
        colors[ImGuiCol_FrameBgActive]         = ImVec4(0.15f, 0.16f, 0.18f, 1.0f);

        colors[ImGuiCol_TitleBg]               = COLOR_HEADER_BG;
        colors[ImGuiCol_TitleBgActive]         = COLOR_HEADER_BG;
        colors[ImGuiCol_TitleBgCollapsed]      = COLOR_HEADER_BG;

        colors[ImGuiCol_MenuBarBg]             = COLOR_SIDEBAR_BG;
        colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.08f, 0.08f, 0.09f, 0.4f);
        colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.20f, 0.21f, 0.24f, 0.8f);
        colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.26f, 0.28f, 0.31f, 1.0f);
        colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.33f, 0.35f, 0.39f, 1.0f);

        colors[ImGuiCol_CheckMark]             = COLOR_BLURPLE;
        colors[ImGuiCol_SliderGrab]            = COLOR_BLURPLE;
        colors[ImGuiCol_SliderGrabActive]      = COLOR_BLURPLE_ACT;

        colors[ImGuiCol_Button]                = COLOR_BLURPLE;
        colors[ImGuiCol_ButtonHovered]         = COLOR_BLURPLE_HOV;
        colors[ImGuiCol_ButtonActive]          = COLOR_BLURPLE_ACT;

        colors[ImGuiCol_Header]                = COLOR_CARD_HOVER;
        colors[ImGuiCol_HeaderHovered]         = COLOR_CARD_ACTIVE;
        colors[ImGuiCol_HeaderActive]          = COLOR_BLURPLE;

        colors[ImGuiCol_Separator]             = COLOR_BORDER_SUBTLE;
        colors[ImGuiCol_SeparatorHovered]      = COLOR_BLURPLE;
        colors[ImGuiCol_SeparatorActive]       = COLOR_BLURPLE_ACT;

        colors[ImGuiCol_ResizeGrip]            = ImVec4(0.00f, 0.00f, 0.00f, 0.0f);
        colors[ImGuiCol_ResizeGripHovered]     = COLOR_BLURPLE_HOV;
        colors[ImGuiCol_ResizeGripActive]      = COLOR_BLURPLE_ACT;

        colors[ImGuiCol_ModalWindowDimBg]     = ImVec4(0.0f, 0.0f, 0.0f, 0.70f);
    }

    inline void LoadFonts(ImGuiIO& io) {
        const char* regularFont = "C:\\Windows\\Fonts\\segoeui.ttf";
        const char* boldFont    = "C:\\Windows\\Fonts\\segoeuib.ttf";

        if (!std::filesystem::exists(regularFont)) regularFont = "C:\\Windows\\Fonts\\arial.ttf";
        if (!std::filesystem::exists(boldFont)) boldFont = regularFont;

        if (std::filesystem::exists(regularFont)) {
            FontRegular = io.Fonts->AddFontFromFileTTF(regularFont, 16.0f);
            FontSmall   = io.Fonts->AddFontFromFileTTF(regularFont, 13.0f);
        } else {
            FontRegular = io.Fonts->AddFontDefault();
            FontSmall   = FontRegular;
        }

        if (std::filesystem::exists(boldFont)) {
            FontBold    = io.Fonts->AddFontFromFileTTF(boldFont, 16.0f);
            FontHeader  = io.Fonts->AddFontFromFileTTF(boldFont, 20.0f);
            FontTitle   = io.Fonts->AddFontFromFileTTF(boldFont, 26.0f);
        } else {
            FontBold    = FontRegular;
            FontHeader  = FontRegular;
            FontTitle   = FontRegular;
        }
    }

    // Desenha Avatar Circular com Imagem de Textura ou Letra e Indicador de Presença estilo Discord
    inline void DrawAvatar(ImDrawList* dl, ImVec2 center, float radius, const std::string& name, bool isOnline, bool showStatus = true, void* textureSrv = nullptr, int statusCode = 0) {
        if (textureSrv) {
            ImVec2 pMin(center.x - radius, center.y - radius);
            ImVec2 pMax(center.x + radius, center.y + radius);
            dl->AddImageRounded(reinterpret_cast<ImTextureID>(textureSrv), pMin, pMax, ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, 255), radius);
            dl->AddCircle(center, radius, IM_COL32(88, 101, 242, 120), 0, 1.5f);
        } else {
            static const ImU32 avatarColors[] = {
                IM_COL32(88, 101, 242, 255),  // Blurple
                IM_COL32(35, 165, 90, 255),   // Emerald
                IM_COL32(250, 166, 26, 255),  // Gold/Amber
                IM_COL32(235, 69, 158, 255),  // Pink
                IM_COL32(237, 66, 69, 255),   // Coral
                IM_COL32(0, 168, 252, 255)    // Cyan
            };

            size_t hash = 0;
            for (char c : name) hash = (hash * 31) + static_cast<unsigned char>(c);
            ImU32 bgColor = avatarColors[hash % 6];

            // Círculo principal do avatar
            dl->AddCircleFilled(center, radius, bgColor);

            // Letra inicial maiúscula
            char initial = name.empty() ? '?' : static_cast<char>(toupper(static_cast<unsigned char>(name[0])));
            char initStr[2] = { initial, '\0' };

            ImFont* font = FontBold ? FontBold : FontRegular;
            float fontSize = radius * 1.08f;
            if (font) {
                ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, initStr);
                ImVec2 textPos = ImVec2(center.x - textSize.x * 0.5f, center.y - textSize.y * 0.5f - 1.0f);
                dl->AddText(font, fontSize, textPos, IM_COL32(255, 255, 255, 255), initStr);
            }
        }

        // Indicador de status com recorte circular em volta
        if (showStatus) {
            float statusRadius = radius * 0.33f;
            if (statusRadius < 4.5f) statusRadius = 4.5f;
            ImVec2 statusCenter = ImVec2(center.x + radius * 0.68f, center.y + radius * 0.68f);

            // Borda do status (cor de fundo para recorte estético)
            dl->AddCircleFilled(statusCenter, statusRadius + 2.0f, IM_COL32(27, 28, 30, 255));

            // Indicador de status (Online, Ausente, Ocupado, Invisível)
            ImU32 statusColor = IM_COL32(128, 132, 142, 255);
            if (isOnline) {
                switch (statusCode) {
                    case 0: statusColor = IM_COL32(35, 165, 90, 255);  break; // Online (Verde)
                    case 1: statusColor = IM_COL32(250, 166, 26, 255); break; // Ausente (Amarelo)
                    case 2: statusColor = IM_COL32(237, 66, 69, 255);  break; // Ocupado (Vermelho)
                    case 3: statusColor = IM_COL32(128, 132, 142, 255); break; // Invisível (Cinza)
                    default: statusColor = IM_COL32(35, 165, 90, 255); break;
                }
            }
            dl->AddCircleFilled(statusCenter, statusRadius, statusColor);
        }
    }

    // Desenha Cartão com cantos arredondados e borda sutil
    inline void DrawCard(ImDrawList* dl, ImVec2 p_min, ImVec2 p_max, ImU32 col, float rounding = 8.0f, ImU32 borderCol = IM_COL32(43, 45, 49, 180), float borderSize = 1.0f) {
        dl->AddRectFilled(p_min, p_max, col, rounding);
        if (borderSize > 0.0f && (borderCol & 0xFF000000)) {
            dl->AddRect(p_min, p_max, borderCol, rounding, 0, borderSize);
        }
    }
}
