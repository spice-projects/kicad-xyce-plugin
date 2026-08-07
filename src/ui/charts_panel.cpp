#include <vector>
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/dcclient.h>
#include <wx/defs.h>
#include <wx/display.h>
#include <wx/event.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/settings.h>
#include <wx/window.h>
#endif

#include <algorithm>
#include <cmath>
#include <set>

#include <imgui.h>
#include <implot.h>
#include <spdlog/spdlog.h>

#include "add_plot_dialog.h"
#include "charts_panel.h"
#include "events.h"
#include "fft_dialog.h"
#include "im_context.h"
#include "step_tool_dialog.h"

namespace
{
    enum
    {
        ID_CONTEXT_ZOOM_TO_FIT = wxID_HIGHEST + 100,
        ID_CONTEXT_AUTORANGE,
        ID_CONTEXT_ZOOM_ABSCISSA_EXTENT,

        ID_CONTEXT_ADD_REMOVE_PLOTS,
        ID_CONTEXT_DELETE_ALL_PLOTS,
        ID_CONTEXT_CALCULATE_FFT,
        ID_CONTEXT_OPEN_XYCE_FFT_CALCULATION,
        ID_CONTEXT_STEP_TOOL,
        ID_CONTEXT_ADD_CHART,
        ID_CONTEXT_DELETE_CHART,
        ID_CONTEXT_NEW_WINDOW
    };
}

#ifdef __linux__
ChartsPanel::ChartsPanel(wxWindow* parent, const wxWindowID id) :
    wxGLCanvas(parent, id, nullptr, wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxFULL_REPAINT_ON_RESIZE) {
#else
ChartsPanel::ChartsPanel(wxWindow* parent, const wxWindowID id) :
    wxPanel(parent, id, wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxFULL_REPAINT_ON_RESIZE) {
#endif
    // mouse events
    Bind(wxEVT_MOTION, &ChartsPanel::on_mouse_move, this);
    // Bind(wxEVT_LEFT_DCLICK, &ChartsPanel::on_mouse_button, this);
    Bind(wxEVT_LEFT_DOWN, &ChartsPanel::on_mouse_button, this);
    Bind(wxEVT_LEFT_UP, &ChartsPanel::on_mouse_button, this);
    Bind(wxEVT_RIGHT_DOWN, &ChartsPanel::on_mouse_button, this);
    Bind(wxEVT_RIGHT_UP, &ChartsPanel::on_mouse_button, this);
    Bind(wxEVT_MIDDLE_DOWN, &ChartsPanel::on_mouse_button, this);
    Bind(wxEVT_MIDDLE_UP, &ChartsPanel::on_mouse_button, this);
    Bind(wxEVT_MOUSEWHEEL, &ChartsPanel::on_mouse_wheel, this);
    Bind(wxEVT_KEY_DOWN, &ChartsPanel::on_key_down, this);
    Bind(wxEVT_KEY_UP, &ChartsPanel::on_key_up, this);
    Bind(wxEVT_CHAR, &ChartsPanel::on_character, this);
    Bind(wxEVT_SET_FOCUS, &ChartsPanel::on_set_focus, this);
    Bind(wxEVT_KILL_FOCUS, &ChartsPanel::on_kill_focus, this);
    // other events
    Bind(wxEVT_IDLE, &ChartsPanel::on_idle, this);
    Bind(wxEVT_PAINT, &ChartsPanel::on_paint, this);
    Bind(wxEVT_SIZE, &ChartsPanel::on_size, this);
    Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) { return; });
    // context menu commands
    Bind(wxEVT_CONTEXT_MENU, &ChartsPanel::on_context_menu, this);
    Bind(wxEVT_MENU, &ChartsPanel::on_menu_zoom_to_fit, this, ID_CONTEXT_ZOOM_TO_FIT);
    Bind(wxEVT_MENU, &ChartsPanel::on_menu_autorange, this, ID_CONTEXT_AUTORANGE);
    Bind(wxEVT_MENU, &ChartsPanel::on_menu_zoom_abscissa_extent, this, ID_CONTEXT_ZOOM_ABSCISSA_EXTENT);
    Bind(wxEVT_MENU, &ChartsPanel::on_menu_add_remove_plots, this, ID_CONTEXT_ADD_REMOVE_PLOTS);
    Bind(wxEVT_MENU, &ChartsPanel::on_menu_delete_all_plots, this, ID_CONTEXT_DELETE_ALL_PLOTS);
    Bind(wxEVT_MENU, &ChartsPanel::on_menu_step_tool, this, ID_CONTEXT_STEP_TOOL);
    Bind(wxEVT_MENU, &ChartsPanel::on_menu_add_chart, this, ID_CONTEXT_ADD_CHART);
    Bind(wxEVT_MENU, &ChartsPanel::on_menu_delete_chart, this, ID_CONTEXT_DELETE_CHART);
    Bind(wxEVT_MENU, &ChartsPanel::on_menu_new_window, this, ID_CONTEXT_NEW_WINDOW);
    Bind(wxEVT_MENU, &ChartsPanel::on_menu_calculate_fft, this, ID_CONTEXT_CALCULATE_FFT);
    Bind(wxEVT_MENU, &ChartsPanel::on_menu_open_fft_calculation, this, ID_CONTEXT_OPEN_XYCE_FFT_CALCULATION);
    // fetch the platform's active workspace background color
    wxColour wxBgColor = wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE);
    // convert it to ImVec4
    m_background_color = ImVec4(wxBgColor.Red() / 255.0f, wxBgColor.Green() / 255.0f, wxBgColor.Blue() / 255.0f, 1.0f);
}

ChartsPanel::~ChartsPanel() {
    // terminate
    terminate();
}

void ChartsPanel::initialize_contexts() {
    // contexts are created exactly once for the panel lifetime
    if (m_imgui_context != nullptr)
        return;
    // preserve the context that was active before this panel initialized
    auto* previous_imgui_context = ImGui::GetCurrentContext();
    auto* previous_implot_context = ImPlot::GetCurrentContext();
    // create and retain the ImGui context
    IMGUI_CHECKVERSION();
    m_imgui_context = ImGui::CreateContext();
    // create and retain the matching ImPlot context
    m_implot_context = ImPlot::CreateContext();
    // restore the prior active contexts
    ImPlot::SetCurrentContext(previous_implot_context);
    ImGui::SetCurrentContext(previous_imgui_context);
}

void ChartsPanel::terminate_contexts() {
    // contexts have already been released
    if (m_imgui_context == nullptr)
        return;
    // preserve the active contexts while releasing this panel's contexts
    auto* previous_imgui_context = ImGui::GetCurrentContext();
    auto* previous_implot_context = ImPlot::GetCurrentContext();
    // retain the context addresses while clearing the members below
    auto* imgui_context = static_cast<ImGuiContext*>(m_imgui_context);
    auto* implot_context = static_cast<ImPlotContext*>(m_implot_context);
    // activate this panel's isolated contexts
    ImGui::SetCurrentContext(imgui_context);
    ImPlot::SetCurrentContext(implot_context);
    // destroy the plot context before its ImGui dependency
    ImPlot::DestroyContext(implot_context);
    m_implot_context = nullptr;
    // destroy the ImGui context
    ImGui::DestroyContext(imgui_context);
    m_imgui_context = nullptr;
    // never restore a context that was released above
    ImPlot::SetCurrentContext(previous_implot_context == implot_context ? nullptr : previous_implot_context);
    ImGui::SetCurrentContext(previous_imgui_context == imgui_context ? nullptr : previous_imgui_context);
}

void ChartsPanel::update_delta_time() {
    // read the monotonic clock once for this panel's frame
    const auto current_time = std::chrono::steady_clock::now();
    // use a conventional initial duration before a prior frame exists
    if (m_last_frame_time.time_since_epoch().count() == 0)
        ImGui::GetIO().DeltaTime = 1.0f / 60.0f;
    else
        ImGui::GetIO().DeltaTime = std::chrono::duration<float>(current_time - m_last_frame_time).count();
    // retain the timestamp for the next frame
    m_last_frame_time = current_time;
}

void ChartsPanel::render() {
    // render
    render_frame([this]() -> void {
        // removing padding
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        // panel
        if (ImGui::Begin("Charts Panel", nullptr, ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs)) {
            // check we have charts to render
            if (!m_charts.empty()) {
                // available are
                const ImVec2 total_space = ImGui::GetContentRegionAvail();
                // chart height
                const float height = total_space.y / static_cast<float>(m_charts.size());
                // render charts within native frame
                for (size_t i = 0; i < m_charts.size(); ++i) {
                    // area name
                    auto name = std::format("Chart {}", i);
                    // create child with given height, use the whole area in the horizontal
                    if (ImGui::BeginChild(name.c_str(), ImVec2(0, height), true)) {
                        // check current chart is selected
                        if (i == m_selected_chart_index) {
                            // render chart
                            m_charts[i]->render(m_zoom_selection);
                        }
                        else {
                            // render chart
                            m_charts[i]->render({-1, -1, -1, -1});
                        }
                        // close
                        ImGui::EndChild();
                    }
                }
            }
            // close
            ImGui::End();
        }
        // pop style var
        ImGui::PopStyleVar();
    });
}

void ChartsPanel::on_mouse_move(wxMouseEvent& event) {
    // forward native input to the panel's ImGui context
    process_mouse_event(event);
    // check the user is dragging the mouse
    if (event.LeftIsDown()) {
        // check we are dragging inside a plot
        if (const auto [x1, y1, x2, y2] = m_zoom_selection; x1 >= 0 and y1 >= 0) {
            // mouse x, y
            auto x = static_cast<float>(event.GetX());
            auto y = static_cast<float>(event.GetY());
            // available area in the panel
            const auto& clientRect = GetClientRect();
            // selected chart
            m_selected_chart_index = event.GetY() / (clientRect.height / m_charts.size());
            if (m_selected_chart_index >= m_charts.size())
                return;
            // current chart
            const auto& chart = m_charts[m_selected_chart_index];
            // plot area
            const auto [x_min, y_min, x_max, y_max] = chart->get_plot_rect();
            // check x user mouse position, force it to be within plot area
            if (x < x1 && x < x_min)
                x = x_min;
            else if (x > x_max)
                x = x_max;
            // check y user mouse position, force it to be within plot area
            if (y < y1 && y < y_min)
                y = y_min;
            else if (y > y_max)
                y = y_max;
            // update rect
            m_zoom_selection = {x1, y1, x, y};
            // refresh
            refresh_charts();
        }
    }
    // skip event
    event.Skip();
}

void ChartsPanel::on_mouse_button(wxMouseEvent& event) {
    // forward native input to the panel's ImGui context
    process_mouse_event(event);
    // events when charts are present
    if (m_charts.empty())
        return;
    // available area in the panel
    const auto& clientRect = GetClientRect();
    // check the user pressed the left mouse button
    if (event.LeftDown()) {
        // selected chart
        m_selected_chart_index = event.GetY() / (clientRect.height / m_charts.size());
        if (m_selected_chart_index < m_charts.size()) {
            // mouse x, y
            const auto x = static_cast<float>(event.GetX());
            const auto y = static_cast<float>(event.GetY());
            // selected chart
            m_selected_chart = m_charts[m_selected_chart_index].get();
            // check user is clicking inside a plot area
            if (const auto [x_min, y_min, x_max, y_max] = m_selected_chart->get_plot_rect(); x >= x_min && x <= x_max && y >= y_min && y <= y_max) {
                // capture current point
                m_zoom_selection = {x, y, -1, -1};
            }
            else {
                // reset current point
                m_zoom_selection = {-1, -1, -1, -1};
            }
        }
        // skip event
        event.Skip();
        // exit
        return;
    }
    // check the user released the left button
    if (event.LeftUp()) {
        // check we are dragging inside a plot
        if (const auto [x1, y1, x2, y2] = m_zoom_selection; m_selected_chart && x1 >= 0 and y1 >= 0 && x2 >= 0 && y2 >= 0 && std::abs(x2 - x1) > 10 && std::abs(y2 - y1) > 10) {
            // plot area
            auto [x_min, y_min, x_max, y_max] = m_selected_chart->get_plot_rect();
            // width & height
            const auto width = x_max - x_min;
            const auto height = y_max - y_min;
            // zoom window
            auto z_x1 = ((std::min)(x1, x2) - x_min) / width;
            auto z_y1 = ((std::min)(y1, y2) - y_min) / height;
            auto z_x2 = ((std::max)(x1, x2) - x_min) / width;
            auto z_y2 = ((std::max)(y1, y2) - y_min) / height;
            // update zoom window
            m_zoom_window = {z_x1, z_y1, z_x2, z_y2};
            // loop charts
            for (size_t i = 0; i < m_charts.size(); i++) {
                // chart at i
                const auto& chart = m_charts[i];
                // check if this is the chart that triggered the zoom action
                if (i == m_selected_chart_index) {
                    // log information
                    spdlog::debug("Updating zoom window in chart at index [{}] to [{}, {}, {}, {}]", i, z_x1, z_y1, z_x2, z_y2);
                    // update zoom window in selected chart
                    chart->update_zoom_window(z_x1, z_x2, z_y1, z_y2);
                }
                else {
                    // log information
                    spdlog::debug("Updating zoom window in chart at index [{}] to [{}, {}, {}, {}]", i, z_x1, z_y1, -1, -1);
                    // update horizontal zoom window only, keep vertical zoom as is
                    chart->update_zoom_window(z_x1, z_x2, -1, -1);
                }
            }
        }
        // reset selected chart & selection
        m_selected_chart = nullptr;
        m_zoom_selection = {-1, -1, -1, -1};
        // refresh
        refresh_charts();
        // skip event
        event.Skip();
    }
    // skip event
    event.Skip();
}

void ChartsPanel::on_mouse_wheel(wxMouseEvent& event) {
    // forward native input to the panel's ImGui context
    process_mouse_wheel_event(event);
    // retain wxWidgets' normal event propagation
    event.Skip();
}

void ChartsPanel::on_key_down(wxKeyEvent& event) {
    // forward native input to the panel's ImGui context
    process_key_event(event, true);
    // retain wxWidgets' normal event propagation
    event.Skip();
}

void ChartsPanel::on_key_up(wxKeyEvent& event) {
    // forward native input to the panel's ImGui context
    process_key_event(event, false);
    // retain wxWidgets' normal event propagation
    event.Skip();
}

void ChartsPanel::on_character(wxKeyEvent& event) {
    // forward native text input to the panel's ImGui context
    process_character_event(event);
    // retain wxWidgets' normal event propagation
    event.Skip();
}

void ChartsPanel::on_set_focus(wxFocusEvent& event) {
    // notify the panel's ImGui context that it received focus
    process_focus_event(true);
    // retain wxWidgets' normal event propagation
    event.Skip();
}

void ChartsPanel::on_kill_focus(wxFocusEvent& event) {
    // notify the panel's ImGui context that it lost focus
    process_focus_event(false);
    // retain wxWidgets' normal event propagation
    event.Skip();
}

void ChartsPanel::process_mouse_event(const wxMouseEvent& event) {
    // input can arrive before the first paint initializes the renderer
    if (m_imgui_context == nullptr)
        return;
    // activate this panel's input context
    ContextScope context_scope(*this);
    // add the absolute client position
    ImGui::GetIO().AddMousePosEvent(static_cast<float>(event.GetX()), static_cast<float>(event.GetY()));
    // add the button transition when one occurred
    if (event.LeftDown() || event.LeftUp())
        ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Left, event.LeftDown());
    if (event.RightDown() || event.RightUp())
        ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Right, event.RightDown());
    if (event.MiddleDown() || event.MiddleUp())
        ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Middle, event.MiddleDown());
}

void ChartsPanel::process_mouse_wheel_event(const wxMouseEvent& event) {
    // input can arrive before the first paint initializes the renderer
    if (m_imgui_context == nullptr)
        return;
    // activate this panel's input context
    ContextScope context_scope(*this);
    // scale the wheel rotation to standard ImGui wheel units
    const float wheel = static_cast<float>(event.GetWheelRotation()) / static_cast<float>(event.GetWheelDelta());
    // add vertical or horizontal wheel input
    if (event.GetWheelAxis() == wxMOUSE_WHEEL_HORIZONTAL)
        ImGui::GetIO().AddMouseWheelEvent(wheel, 0.0f);
    else
        ImGui::GetIO().AddMouseWheelEvent(0.0f, wheel);
}

void ChartsPanel::process_key_event(const wxKeyEvent& event, const bool pressed) {
    // input can arrive before the first paint initializes the renderer
    if (m_imgui_context == nullptr)
        return;
    // activate this panel's input context
    ContextScope context_scope(*this);
    // update modifier state before the key transition
    auto& io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiMod_Ctrl, event.ControlDown());
    io.AddKeyEvent(ImGuiMod_Shift, event.ShiftDown());
    io.AddKeyEvent(ImGuiMod_Alt, event.AltDown());
    io.AddKeyEvent(ImGuiMod_Super, event.MetaDown());
    // map wxWidgets virtual key codes to Dear ImGui keys
    const int key_code = event.GetKeyCode();
    if (key_code >= 'a' && key_code <= 'z')
        io.AddKeyEvent(static_cast<ImGuiKey>(ImGuiKey_A + key_code - 'a'), pressed);
    else if (key_code >= 'A' && key_code <= 'Z')
        io.AddKeyEvent(static_cast<ImGuiKey>(ImGuiKey_A + key_code - 'A'), pressed);
    else if (key_code >= '0' && key_code <= '9')
        io.AddKeyEvent(static_cast<ImGuiKey>(ImGuiKey_0 + key_code - '0'), pressed);
    else if (key_code == WXK_SPACE)
        io.AddKeyEvent(ImGuiKey_Space, pressed);
    else if (key_code == WXK_RETURN || key_code == WXK_NUMPAD_ENTER)
        io.AddKeyEvent(ImGuiKey_Enter, pressed);
    else if (key_code == WXK_ESCAPE)
        io.AddKeyEvent(ImGuiKey_Escape, pressed);
    else if (key_code == WXK_TAB)
        io.AddKeyEvent(ImGuiKey_Tab, pressed);
    else if (key_code == WXK_BACK)
        io.AddKeyEvent(ImGuiKey_Backspace, pressed);
    else if (key_code == WXK_DELETE)
        io.AddKeyEvent(ImGuiKey_Delete, pressed);
}

void ChartsPanel::process_character_event(const wxKeyEvent& event) {
    // input can arrive before the first paint initializes the renderer
    if (m_imgui_context == nullptr)
        return;
    // use the Unicode code point supplied by wxWidgets
    const wxChar character = event.GetUnicodeKey();
    if (character == WXK_NONE)
        return;
    // activate this panel's input context
    ContextScope context_scope(*this);
    // add UTF-32 text input
    ImGui::GetIO().AddInputCharacter(static_cast<unsigned int>(character));
}

void ChartsPanel::process_focus_event(const bool focused) {
    // focus can change before the first paint initializes the renderer
    if (m_imgui_context == nullptr)
        return;
    // activate this panel's input context
    ContextScope context_scope(*this);
    // notify ImGui of the focus transition
    ImGui::GetIO().AddFocusEvent(focused);
}

void ChartsPanel::on_idle(wxIdleEvent& event) {
    // check flag is set
    if (m_render_chart_frames > 0) {
        // decrease counter
        m_render_chart_frames--;
        // render charts panel
        render();
        // request another idle event to render the next frame
        if (m_render_chart_frames > 0)
            event.RequestMore();
    }
    // skip event
    event.Skip();
}

void ChartsPanel::on_paint(wxPaintEvent& event) {
    // required call
    wxPaintDC dc(this);
    // initialize widget (only the first time)
    initialize();
    // indicate we need to render charts
    m_render_chart_frames = 1;
    // skip event
    event.Skip();
}

void ChartsPanel::on_size(wxSizeEvent& event) {
    // recompute the decimation target for the new panel width
    update_decimation_target();
    // check we have initialized
    if (m_charts_panel) {
        // update bounds in the native layer
        if (update_bounds()) {
            // refresh charts
            refresh_charts();
        }
    }
    // skip even
    event.Skip();
}

void ChartsPanel::on_context_menu(const wxContextMenuEvent& event) {
    // context menu only available when charts are present
    if (m_charts.empty())
        return;
    // mouse position where the user right-clicked
    wxPoint mousePosition = event.GetPosition();
    if (mousePosition == wxDefaultPosition) {
        // current panel size
        wxSize panel_size = GetSize();
        // use center of the panel
        mousePosition = wxPoint(panel_size.x / 2, panel_size.y / 2);
    }
    else {
        // convert screen coordinates from the event to local window coordinates
        mousePosition = ScreenToClient(mousePosition);
    }
    // available area in the panel
    const auto& clientRect = GetClientRect();
    // selected chart
    m_selected_chart_index = mousePosition.y / (clientRect.height / m_charts.size());
    if (m_selected_chart_index >= m_charts.size()) {
        // reset selected chart
        m_selected_chart = nullptr;
        // exit
        return;
    }
    // chart at index
    m_selected_chart = m_charts[m_selected_chart_index].get();
    // log information
    spdlog::debug("User requested context menu at index: {}", m_selected_chart_index);
    // create context menu
    wxMenu contextMenu;
    // menu items
    contextMenu.Append(ID_CONTEXT_ZOOM_TO_FIT, "Zoom to Fit");
    contextMenu.Append(ID_CONTEXT_AUTORANGE, "Autorange");
    contextMenu.Append(ID_CONTEXT_ZOOM_ABSCISSA_EXTENT, "Zoom Abscissa Extent");
    contextMenu.AppendSeparator();
    contextMenu.Append(ID_CONTEXT_ADD_REMOVE_PLOTS, "Add/Remove Plots");
    contextMenu.Append(ID_CONTEXT_DELETE_ALL_PLOTS, "Delete All Plots");
    // tools block, conditional on one of them being available
    if (m_expression_manager->abscissa().unit() == "s" || m_step_information->length() > 1) {
        // separator
        contextMenu.AppendSeparator();
        // FFT, transient analysis
        if (m_expression_manager->abscissa().unit() == "s") {
            // calculate FFT always available (user can select any real-valued expression to compute FFT)
            contextMenu.Append(ID_CONTEXT_CALCULATE_FFT, "Calculate FFT on selected plots");
            // open Xyce FFT calculation only when FFT output files were produced by the last run
            if (!m_fft_calculation_files.empty())
                contextMenu.Append(ID_CONTEXT_OPEN_XYCE_FFT_CALCULATION, "Open Xyce FFT Calculation");
        }
        // append step tool only on multiple steps (otherwise it is not useful)
        if (m_step_information->length() > 1)
            contextMenu.Append(ID_CONTEXT_STEP_TOOL, "Step Tool...");
        // TODO: add Smith chart tool when available
        // contextMenu.Append(ID_CONTEXT_OPTION_2, "Smith Chart...");
    }
    contextMenu.AppendSeparator();
    contextMenu.Append(ID_CONTEXT_ADD_CHART, "Add Chart");
    contextMenu.Append(ID_CONTEXT_DELETE_CHART, "Delete Chart");
    contextMenu.AppendSeparator();
    contextMenu.Append(ID_CONTEXT_NEW_WINDOW, "New Window");
    // display menu (blocking)
    PopupMenu(&contextMenu, mousePosition);
}

void ChartsPanel::on_menu_zoom_to_fit(wxCommandEvent&) {
    // check we have a selected chart
    if (m_selected_chart != nullptr) {
        // log information
        spdlog::debug("User requested zoom to fit on chart at index: {}", m_selected_chart_index);
        // loop charts
        for (size_t i = 0; i < m_charts.size(); i++) {
            // chart at i
            const auto& chart = m_charts[i];
            // check if this is the chart that triggered the zoom to fit action
            if (i == m_selected_chart_index) {
                // reset zoom window
                chart->reset_zoom_window(true, true);
                // next
                continue;
            }
            // update horizontal zoom window only, keep vertical zoom as is
            chart->reset_zoom_window(true, false);
        }
        // reset selected chart
        m_selected_chart = nullptr;
        // refresh
        refresh_charts();
    }
}

void ChartsPanel::on_menu_autorange(wxCommandEvent&) {
    // check we have a selected chart
    if (m_selected_chart != nullptr) {
        // log information
        spdlog::debug("User requested autorange on chart at index: {}", m_selected_chart_index);
        // reset zoom window
        m_selected_chart->reset_zoom_window(false, true);
        // reset selected chart
        m_selected_chart = nullptr;
        // refresh
        refresh_charts();
    }
}

void ChartsPanel::on_menu_zoom_abscissa_extent(wxCommandEvent&) {
    // check we have a selected chart
    if (m_selected_chart != nullptr) {
        // log information
        spdlog::debug("User requested zoom abscissa extent on chart at index: {}", m_selected_chart_index);
        // loop charts
        for (const auto& chart : m_charts) {
            // reset zoom window
            chart->reset_zoom_window(true, false);
        }
        // reset selected chart
        m_selected_chart = nullptr;
        // refresh
        refresh_charts();
    }
}

void ChartsPanel::on_menu_add_remove_plots(wxCommandEvent&) {
    // check we have a selected chart
    if (m_selected_chart != nullptr) {
        // log information
        spdlog::debug("User requested add/remove plots on chart at index: {}", m_selected_chart_index);
        // create dialog
        AddPlotDialog dialog(this, m_expression_manager, m_selected_chart->selected_expressions());
        // center on screen
        dialog.Centre(wxCENTER_ON_SCREEN);
        // show and wait for ok
        if (dialog.ShowModal() == wxID_OK) {
            // update chart with selected expressions
            m_selected_chart->plot_series(dialog.selected_expressions());
            // refresh
            refresh_charts();
        }
        // reset selected chart
        m_selected_chart = nullptr;
    }
}

void ChartsPanel::on_menu_delete_all_plots(wxCommandEvent&) {
    // check we have a selected chart
    if (m_selected_chart != nullptr) {
        // log information
        spdlog::debug("User requested deleting all plots on chart at index: {}", m_selected_chart_index);
        // clear chart
        m_selected_chart->clear();
        // reset selected chart
        m_selected_chart = nullptr;
        // refresh
        refresh_charts();
    }
}

void ChartsPanel::on_menu_step_tool(wxCommandEvent&) {
    // check we have a selected chart
    if (m_selected_chart != nullptr) {
        // log information
        spdlog::debug("User requested step tool on chart at index: {}", m_selected_chart_index);
        // create dialog
        StepToolDialog dialog(this, m_step_information, m_selected_chart->selected_steps());
        // center on screen
        dialog.Centre(wxCENTER_ON_SCREEN);
        // show and wait for ok
        if (dialog.ShowModal() == wxID_OK) {
            // refresh
            refresh_charts();
        }
        // reset selected chart
        m_selected_chart = nullptr;
    }
}

void ChartsPanel::on_menu_add_chart(wxCommandEvent&) {
    // check we have a selected chart
    if (m_selected_chart != nullptr) {
        // log information
        spdlog::debug("User requested adding a new chart at index: {}", m_selected_chart_index);
        // add a new chart with no pre-populated expressions
        add_chart();
        // reset selected chart
        m_selected_chart = nullptr;
        // refresh
        refresh_charts();
    }
}

void ChartsPanel::on_menu_delete_chart(wxCommandEvent&) {
    // check we have a selected chart
    if (m_selected_chart != nullptr) {
        // log information
        spdlog::debug("User requested deleting chart at index: {}", m_selected_chart_index);
        // delete chart at index
        m_charts.erase(m_charts.begin() + static_cast<int>(m_selected_chart_index));
        // ensure at leat one chart in panel
        if (m_charts.empty())
            add_chart();
        // reset selected chart
        m_selected_chart = nullptr;
        // refresh
        refresh_charts();
    }
}

void ChartsPanel::on_menu_new_window(wxCommandEvent& event) {
    // create the custom command event instance
    wxCommandEvent e(wxEVT_NEW_WINDOW, GetId());
    // set the event's event object to this panel
    e.SetEventObject(this);
    // fire the event up to the parent
    GetEventHandler()->ProcessEvent(e);
    // skip event
    event.Skip();
}

void ChartsPanel::on_menu_open_fft_calculation(wxCommandEvent& event) {
    // create the custom command event instance
    wxCommandEvent e(wxEVT_OPEN_XYCE_FFT_CALCULATION, GetId());
    // set the event's event object to this panel
    e.SetEventObject(this);
    // fire the event up to the parent
    GetEventHandler()->ProcessEvent(e);
    // skip event
    event.Skip();
}

void ChartsPanel::on_menu_calculate_fft(wxCommandEvent&) {
    // check we have a selected chart
    if (m_selected_chart == nullptr) {
        // exit
        return;
    }
    // log information
    spdlog::debug("User requested Calculate FFT on chart at index: {}", m_selected_chart_index);
    // compute min/max abscissa values
    double min_abscissa_value = m_step_information->abscissa_left_value();
    double max_abscissa_value = m_step_information->abscissa_right_value();
    // compute current chart zoom window
    const auto& zoom_window = m_selected_chart->zoom_window();
    // left and right ratios of the zoom window in the abscissa range
    double x_left_ratio = std::get<0>(zoom_window);
    double x_right_ratio = std::get<2>(zoom_window);
    // apply selected chart zoom (use full range if zoom is reset)
    double min_abscissa_value_zoomed = min_abscissa_value + (x_left_ratio < 0.0 ? 0.0 : x_left_ratio) * (max_abscissa_value - min_abscissa_value);
    double max_abscissa_value_zoomed = min_abscissa_value + (x_right_ratio < 0.0 ? 1.0 : x_right_ratio) * (max_abscissa_value - min_abscissa_value);
    // current expressions plotted on the selected chart
    auto plotted_expressions = m_selected_chart->selected_expressions();
    // open FFT settings dialog (pre-select all real expressions)
    FftDialog dialog(this, m_expression_manager, std::vector<AnyExpression*>(plotted_expressions.begin(), plotted_expressions.end()), min_abscissa_value, max_abscissa_value, min_abscissa_value_zoomed, max_abscissa_value_zoomed);
    // center in the screen
    dialog.Centre(wxCENTER_ON_SCREEN);
    // open modal
    if (dialog.ShowModal() != wxID_OK) {
        // exit
        return;
    }
    // get dialog results
    auto selected_expressions = std::vector(dialog.selected_expressions());
    double from_abscissa_value = dialog.from_index();
    double to_abscissa_value = dialog.to_index();
    fft::FftParameters fft_params = dialog.parameters();
    // validate selection
    if (selected_expressions.empty()) {
        // show error message
        wxMessageBox("No expressions selected.", "Error", wxOK | wxICON_ERROR, this);
        // exit
        return;
    }
    // list of frequency bins for each step, to be concatenated across steps later
    std::vector<std::vector<double>> frequency_chunks;
    // fft data chunks for each expression, to be concatenated across steps later
    std::vector<std::vector<std::vector<double>>> fft_chunks(selected_expressions.size());
    // fft step & step indices
    std::vector<size_t> fft_steps;
    std::vector<std::pair<size_t, size_t>> fft_abscissa_indices;
    // step abscissa value ranges
    std::vector<std::pair<double, double>> fft_abscissa_value_ranges;
    // fft step index offset
    size_t fft_offset = 0;
    // loop steps
    for (size_t step = 0; step < m_step_information->length(); ++step) {
        // abscissa values for this step — zero copy per-step view
        std::span<const double> step_abscissa = m_expression_manager->abscissa().step_data(step);
        // find indices corresponding to the selected abscissa range
        auto it_left = std::lower_bound(step_abscissa.begin(), step_abscissa.end(), from_abscissa_value);
        auto it_right = std::upper_bound(step_abscissa.begin(), step_abscissa.end(), to_abscissa_value);
        // from and to indeces for the selected abscissa range (inclusive of from_index, exclusive of to_index)
        size_t from_index = static_cast<size_t>(std::distance(step_abscissa.begin(), it_left));
        size_t to_index = static_cast<size_t>(std::distance(step_abscissa.begin(), it_right));
        // require at least 2 samples
        if (to_index - from_index < 2) {
            // log information
            spdlog::warn("Skipping FFT for chart {} step {}: selected range has fewer than 2 samples", m_selected_chart_index, step);
            // next
            continue;
        }
        // expressions in this step
        std::vector<std::span<const double>> y_matrix;
        // reserve memory
        y_matrix.reserve(selected_expressions.size());
        // loop expression
        for (AnyExpression* expression : selected_expressions) {
            // ensure it is a real-valued expression (Expression<double>)
            if (std::holds_alternative<Expression<double>>(*expression)) {
                // get the Expression<double> variant
                auto& double_expr = std::get<Expression<double>>(*expression);
                // step data
                auto y_data = double_expr.step_data(step);
                // append the selected range to the y_matrix
                y_matrix.push_back(y_data.subspan(from_index, to_index - from_index));
            }
        }
        // skip empty matrix (should not happen since we filtered for real expressions)
        if (y_matrix.empty())
            continue;
        // extract the x interval for the FFT
        auto x_interval = step_abscissa.subspan(from_index, to_index - from_index);
        try {
            // compute FFT, all expressions in y_matrix are processed together for this step
            auto result = fft::compute_fft_many(x_interval, y_matrix, fft_params.np, fft_params.window, fft_params.format, 0, x_interval.size() - 1, fft_params.output, fft_params.keep_dc);
            // check if the frequency axis is empty
            if (result.frequencies.empty()) {
                // log information
                spdlog::error("FFT computation returned an empty frequency axis for chart {} step {}", m_selected_chart_index, step);
                // exit
                return;
            }
            // store frequency bins
            frequency_chunks.push_back(result.frequencies);
            // store per-expression FFT values
            for (size_t i = 0; i < result.values.size(); ++i)
                fft_chunks[i].push_back(result.values[i]);
            // store step output slice
            fft_abscissa_indices.emplace_back(fft_offset, fft_offset + result.frequencies.size());
            // update offset
            fft_offset += result.frequencies.size();
            // append step
            fft_steps.push_back(step);
            // store abscissa value range for this step
            fft_abscissa_value_ranges.emplace_back(result.frequencies[0], result.frequencies.back());
        }
        catch (const std::exception& e) {
            // log information
            spdlog::error("FFT computation failed for chart {} step {}: {}", m_selected_chart_index, step, e.what());
            // exit
            return;
        }
    }

    // require at least one processed step
    if (fft_steps.empty()) {
        spdlog::warn("FFT computation skipped: no step has at least 2 samples in the selected range");
        return;
    }
    // build expression name for the title
    std::string fft_title = "FFT - ";
    // loop selected expressions to build the title
    for (size_t i = 0; i < selected_expressions.size(); ++i) {
        // real expression
        auto& real_expression = std::get<Expression<double>>(*selected_expressions[i]);
        // append separator
        if (i > 0)
            fft_title += ", ";
        // append expression name
        fft_title += real_expression.name();
    }
    // build FFT expressions using the Expression<double> constructor with step slices
    std::vector<AnyExpression> fft_expressions;
    {
        // create flat frequency data
        std::vector<double> freq_data;
        // reserve memory for the concatenated frequency data
        freq_data.reserve(fft_offset);
        // concatenate frequency chunks across steps
        for (const auto& chunk : frequency_chunks)
            freq_data.insert(freq_data.end(), chunk.begin(), chunk.end());
        // append expression for frequency abscissa with unit "Hz" and the corresponding step slices
        fft_expressions.emplace_back(Expression<double>("Frequency", std::move(freq_data), fft_abscissa_indices, "Hz"));
    }
    // suggested plots
    std::vector<std::vector<std::string>> suggested_plots;
    // create FFT expressions
    for (size_t i = 0; i < selected_expressions.size(); ++i) {
        // current expression
        auto* expression = selected_expressions[i];
        // real expression
        auto& real_expression = std::get<Expression<double>>(*expression);
        // determine unit
        std::string unit;
        if (fft_params.output == fft::FftOutput::PHASE)
            unit = "°";
        else if (fft_params.output == fft::FftOutput::MAGNITUDE_DB)
            unit = "dB";
        else
            unit = real_expression.unit();
        // expression name for the FFT result
        auto expr_name = "FFT(" + real_expression.name() + ")";
        // create flat data
        std::vector<double> data;
        // reserve memory for the concatenated FFT data
        data.reserve(fft_offset);
        // concatenate FFT chunks across steps
        for (const auto& chunk : fft_chunks[i])
            data.insert(data.end(), chunk.begin(), chunk.end());
        // append expression name to suggested plots (a maximum of three expressions are suggested for plotting)
        if (suggested_plots.size() < 3)
            suggested_plots.push_back({expr_name});
        // append expression for the FFT result with the corresponding step slices
        fft_expressions.emplace_back(Expression<double>(expr_name, std::move(data), fft_abscissa_indices, unit));
    }
    // step information for FFT output (only include steps that were processed)
    StepInformation fft_step_information(m_step_information->keys(), m_step_information->values(), fft_abscissa_value_ranges);
    // build expression manager (moves from expr_list)
    ExpressionManager fft_expression_manager(fft_expressions, fft_abscissa_indices);
    // create raw file with FFT results
    auto fft_raw = std::make_shared<XyceOutputFile>("", fft_title, false, std::move(fft_step_information), AbscissaScale::LINEAR, std::move(fft_expression_manager), nullptr, suggested_plots);
    // spawn a new window with the FFT results via callback
    if (m_fft_result_callback)
        m_fft_result_callback(fft_raw);
    // reset selected chart
    m_selected_chart = nullptr;
}

void ChartsPanel::update(ExpressionManager& expression_manager, const StepInformation& step_information, const std::string& abscissa_label, const AbscissaScale abscissa_scale, const std::vector<std::vector<std::string>>& suggested_plots) {
    // recompute the decimation target from the current panel size and display scale
    update_decimation_target();
    // update fields
    m_expression_manager = &expression_manager;
    m_step_information = &step_information;
    m_abscissa_label = abscissa_label;
    m_abscissa_scale = abscissa_scale;
    // check charts are present, if not add one
    if (!m_charts.empty()) {
        // loop charts
        for (const auto& chart : m_charts) {
            // update chart with new information
            chart->update(m_expression_manager, m_step_information, m_abscissa_label, m_abscissa_scale);
        }
    }
    else if (!suggested_plots.empty()) {
        // create one chart per suggested plot group
        for (const auto& plot_names : suggested_plots) {
            // add a chart for the group
            auto* chart = add_chart();
            // resolved expressions for this group
            std::set<AnyExpression*> resolved_expressions;
            // resolve each expression name to its pointer
            for (const auto& name : plot_names) {
                // evaluate the expression name
                auto* expression = m_expression_manager->evaluate(name);
                // check it resolved to an existing expression
                if (expression == nullptr) {
                    // log information
                    spdlog::warn("Suggested plot expression '{}' not found in expression manager", name);
                    // skip unresolved expression
                    continue;
                }
                // append resolved expression
                resolved_expressions.insert(expression);
            }
            // plot the resolved expressions on the chart
            chart->plot_series(resolved_expressions);
        }
    }
    else {
        // add a new chart with no pre-populated expressions
        add_chart();
    }
    // refresh
    refresh_charts();
}

Chart* ChartsPanel::add_chart() {
    // create chart and append it to vector
    m_charts.push_back(std::make_unique<Chart>(m_expression_manager, m_step_information, m_abscissa_label, m_abscissa_scale, m_decimate_target));
    // chart
    auto& chart = m_charts[m_charts.size() - 1];
    // plot series (will do nothing, but will set the correct abscissa for the zoom window)
    chart->plot_series({});
    // exit
    return chart.get();
}

void ChartsPanel::delete_all_charts() {
    // simple, clean vector
    m_charts.clear();
    // refresh
    refresh_charts();
}

size_t ChartsPanel::compute_decimation_target() const {
    // floor to keep the decimation algorithm meaningful on very small windows
    constexpr size_t min_target = 500;
    // fallback when no usable size or scale is available (headless / early startup)
    constexpr size_t fallback_target = 9600;
    // panel width in logical pixels and the per-window display scale factor
    const int logical_width = GetClientSize().GetWidth();
    const double scale = GetDPIScaleFactor();
    // physical pixel width of the chart plot area
    if (logical_width > 0 && scale > 0.0)
        return (std::max)(min_target, static_cast<size_t>(std::lround(static_cast<double>(logical_width) * scale)));
    // fall back to the geometry of the display this panel is on
    const auto area = wxDisplay(wxDisplay::GetFromWindow(this)).GetClientArea();
    if (area.GetWidth() > 0 && scale > 0.0)
        return (std::max)(min_target, static_cast<size_t>(std::lround(static_cast<double>(area.GetWidth()) * scale)));
    // conservative fallback
    return fallback_target;
}

void ChartsPanel::update_decimation_target() {
    // recompute the target for the current panel size and display scale
    const size_t target = compute_decimation_target();
    // check the target changed
    if (target == m_decimate_target)
        return;
    // store it
    m_decimate_target = target;
    // propagate it to existing charts so future decimations use the new target
    for (const auto& chart : m_charts)
        chart->set_decimate_target(target);
}

void ChartsPanel::refresh_charts(int frames) { m_render_chart_frames = frames; }
