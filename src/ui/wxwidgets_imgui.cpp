#include <imgui.h>
#include <imgui_internal.h>

#include <wx/wx.h>

#include "wxwidgets_imgui.h"

namespace
{
    static ImVec4 to_imgui_color(const wxColour& color) {
        // convert wxColour to ImVec4
        return ImVec4(color.Red() / 255.0f, color.Green() / 255.0f, color.Blue() / 255.0f, color.Alpha() / 255.0f);
    }

    static ImVec4 to_sys_color(wxSystemColour index, wxSystemColour fallback_index) {
        // get the system color for the given index
        const wxColour c = wxSystemSettings::GetColour(index);
        if (c.IsOk())
            return to_imgui_color(c);
        // fallback color if the requested system color is not available
        const wxColour fallback = wxSystemSettings::GetColour(fallback_index);
        if (fallback.IsOk())
            return to_imgui_color(fallback);
        // last resort remains system-derived instead of using hardcoded RGB values.
        return to_imgui_color(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    }

    static ImVec4 with_alpha(const ImVec4& color, float alpha) {
        // return a new ImVec4 with the same RGB values but with the specified alpha value
        return ImVec4(color.x, color.y, color.z, alpha);
    }
}

void PlatformStyle(ImGuiStyle* dst) {
    // get default style if not provided
    ImGuiStyle* style = dst ? dst : &ImGui::GetStyle();
    // style colors
    ImVec4* colors = style->Colors;

    // system colors
    const ImVec4 window_text = to_sys_color(wxSYS_COLOUR_WINDOWTEXT, wxSYS_COLOUR_BTNTEXT);
    const ImVec4 gray_text = to_sys_color(wxSYS_COLOUR_GRAYTEXT, wxSYS_COLOUR_BTNSHADOW);
    const ImVec4 window_bg = to_sys_color(wxSYS_COLOUR_WINDOW, wxSYS_COLOUR_BTNFACE);
    const ImVec4 panel_bg = to_sys_color(wxSYS_COLOUR_BTNFACE, wxSYS_COLOUR_WINDOW);
    const ImVec4 border = to_sys_color(wxSYS_COLOUR_ACTIVEBORDER, wxSYS_COLOUR_BTNSHADOW);
    const ImVec4 highlight = to_sys_color(wxSYS_COLOUR_HIGHLIGHT, wxSYS_COLOUR_HOTLIGHT);
    const ImVec4 highlight_text = to_sys_color(wxSYS_COLOUR_HIGHLIGHTTEXT, wxSYS_COLOUR_WINDOWTEXT);
    const ImVec4 hotlight = to_sys_color(wxSYS_COLOUR_HOTLIGHT, wxSYS_COLOUR_HIGHLIGHT);

    // ImGui colors
    colors[ImGuiCol_Text] = window_text;
    colors[ImGuiCol_TextDisabled] = gray_text;
    colors[ImGuiCol_WindowBg] = with_alpha(window_bg, 0.94f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = with_alpha(panel_bg, 0.94f);
    colors[ImGuiCol_Border] = with_alpha(border, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = with_alpha(ImLerp(panel_bg, highlight, 0.40f), 0.54f);
    colors[ImGuiCol_FrameBgHovered] = with_alpha(highlight, 0.40f);
    colors[ImGuiCol_FrameBgActive] = with_alpha(highlight, 0.67f);
    colors[ImGuiCol_TitleBg] = to_sys_color(wxSYS_COLOUR_ACTIVECAPTION, wxSYS_COLOUR_BTNFACE);
    colors[ImGuiCol_TitleBgActive] = to_sys_color(wxSYS_COLOUR_GRADIENTACTIVECAPTION, wxSYS_COLOUR_ACTIVECAPTION);
    colors[ImGuiCol_TitleBgCollapsed] = with_alpha(to_sys_color(wxSYS_COLOUR_INACTIVECAPTION, wxSYS_COLOUR_ACTIVECAPTION), 0.51f);
    colors[ImGuiCol_MenuBarBg] = to_sys_color(wxSYS_COLOUR_MENUBAR, wxSYS_COLOUR_BTNFACE);
    colors[ImGuiCol_ScrollbarBg] = with_alpha(to_sys_color(wxSYS_COLOUR_SCROLLBAR, wxSYS_COLOUR_BTNFACE), 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = to_sys_color(wxSYS_COLOUR_3DSHADOW, wxSYS_COLOUR_BTNSHADOW);
    colors[ImGuiCol_ScrollbarGrabHovered] = to_sys_color(wxSYS_COLOUR_BTNSHADOW, wxSYS_COLOUR_3DSHADOW);
    colors[ImGuiCol_ScrollbarGrabActive] = to_sys_color(wxSYS_COLOUR_3DDKSHADOW, wxSYS_COLOUR_BTNSHADOW);
    colors[ImGuiCol_CheckMark] = highlight;
    colors[ImGuiCol_CheckboxSelectedBg] = ImLerp(colors[ImGuiCol_FrameBg], colors[ImGuiCol_FrameBgHovered], 0.65f);
    colors[ImGuiCol_SliderGrab] = ImLerp(highlight, panel_bg, 0.15f);
    colors[ImGuiCol_SliderGrabActive] = highlight;
    colors[ImGuiCol_Button] = with_alpha(highlight, 0.40f);
    colors[ImGuiCol_ButtonHovered] = with_alpha(hotlight, 1.00f);
    colors[ImGuiCol_ButtonActive] = highlight;
    colors[ImGuiCol_Header] = with_alpha(highlight, 0.31f);
    colors[ImGuiCol_HeaderHovered] = with_alpha(hotlight, 0.80f);
    colors[ImGuiCol_HeaderActive] = with_alpha(hotlight, 1.00f);
    colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered] = with_alpha(hotlight, 0.78f);
    colors[ImGuiCol_SeparatorActive] = hotlight;
    colors[ImGuiCol_ResizeGrip] = with_alpha(highlight, 0.20f);
    colors[ImGuiCol_ResizeGripHovered] = with_alpha(hotlight, 0.67f);
    colors[ImGuiCol_ResizeGripActive] = with_alpha(hotlight, 0.95f);
    colors[ImGuiCol_InputTextCursor] = colors[ImGuiCol_Text];
    colors[ImGuiCol_TabHovered] = colors[ImGuiCol_HeaderHovered];
    colors[ImGuiCol_Tab] = ImLerp(colors[ImGuiCol_Header], colors[ImGuiCol_TitleBgActive], 0.80f);
    colors[ImGuiCol_TabSelected] = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
    colors[ImGuiCol_TabSelectedOverline] = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TabDimmed] = ImLerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
    colors[ImGuiCol_TabDimmedSelected] = ImLerp(colors[ImGuiCol_TabSelected], colors[ImGuiCol_TitleBg], 0.40f);
    colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
    colors[ImGuiCol_PlotLines] = gray_text;
    colors[ImGuiCol_PlotLinesHovered] = with_alpha(hotlight, 1.00f);
    colors[ImGuiCol_PlotHistogram] = highlight;
    colors[ImGuiCol_PlotHistogramHovered] = with_alpha(hotlight, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImLerp(panel_bg, border, 0.35f);
    colors[ImGuiCol_TableBorderStrong] = border;
    colors[ImGuiCol_TableBorderLight] = ImLerp(border, panel_bg, 0.45f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = with_alpha(gray_text, 0.06f);
    colors[ImGuiCol_TextLink] = colors[ImGuiCol_HeaderActive];
    colors[ImGuiCol_TextSelectedBg] = with_alpha(highlight, 0.35f);
    colors[ImGuiCol_TreeLines] = colors[ImGuiCol_Border];
    colors[ImGuiCol_DragDropTarget] = with_alpha(highlight_text, 0.90f);
    colors[ImGuiCol_DragDropTargetBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_UnsavedMarker] = window_text;
    colors[ImGuiCol_NavCursor] = highlight;
    colors[ImGuiCol_NavWindowingHighlight] = with_alpha(highlight_text, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = with_alpha(panel_bg, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = with_alpha(panel_bg, 0.35f);
}
