#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/dcclient.h>
#include <wx/defs.h>
#include <wx/event.h>
#include <wx/menu.h>
#include <wx/settings.h>
#include <wx/window.h>
#endif

#include <imgui.h>
#include <spdlog/spdlog.h>

#include "add_plot_dialog.h"
#include "charts_panel.h"
#include "events.h"
#include "step_tool_dialog.h"

namespace
{
    enum
    {
        ID_CONTEXT_ZOOM_TO_FIT = wxID_HIGHEST + 100,
        ID_CONTEXT_AUTORANGE,
        ID_CONTEXT_ZOOM_ABSCISSA_EXTENT,
        ID_CONTEXT_OPTION_2,

        ID_CONTEXT_ADD_REMOVE_PLOTS,
        ID_CONTEXT_DELETE_ALL_PLOTS,
        ID_CONTEXT_STEP_TOOL,
        ID_CONTEXT_ADD_CHART,
        ID_CONTEXT_DELETE_CHART,
        ID_CONTEXT_NEW_WINDOW
    };
}

ChartsPanel::ChartsPanel(wxWindow* parent, const wxWindowID id) :
    wxPanel(parent, id, wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxFULL_REPAINT_ON_RESIZE) {
    // mouse events
    Bind(wxEVT_MOTION, &ChartsPanel::on_mouse_move, this);
    // Bind(wxEVT_LEFT_DCLICK, &ChartsPanel::on_mouse_button, this);
    Bind(wxEVT_LEFT_DOWN, &ChartsPanel::on_mouse_button, this);
    Bind(wxEVT_LEFT_UP, &ChartsPanel::on_mouse_button, this);
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
    // fetch the platform's active workspace background color
    wxColour wxBgColor = wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE);
    // convert it to ImVec4
    m_background_color = ImVec4(wxBgColor.Red() / 255.0f, wxBgColor.Green() / 255.0f, wxBgColor.Blue() / 255.0f, 1.0f);
}

ChartsPanel::~ChartsPanel() {
    // terminate
    terminate();
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
            auto z_x1 = (std::min(x1, x2) - x_min) / width;
            auto z_y1 = (std::min(y1, y2) - y_min) / height;
            auto z_x2 = (std::max(x1, x2) - x_min) / width;
            auto z_y2 = (std::max(y1, y2) - y_min) / height;
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
        // FFT
        if (m_expression_manager->abscissa().unit() == "s") {
            // calculate FFT always available
            contextMenu.Append(ID_CONTEXT_OPTION_2, "Calculate FFT on selected plots");
            contextMenu.Append(ID_CONTEXT_OPTION_2, "Open Xyce FFT Calculation");
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

void ChartsPanel::on_menu_zoom_to_fit(wxCommandEvent& event) {
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
    // skip event
    event.Skip();
}

void ChartsPanel::on_menu_autorange(wxCommandEvent& event) {
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
    // skip event
    event.Skip();
}

void ChartsPanel::on_menu_zoom_abscissa_extent(wxCommandEvent& event) {
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
    // skip event
    event.Skip();
}

void ChartsPanel::on_menu_add_remove_plots(wxCommandEvent& event) {
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
    // skip event
    event.Skip();
}

void ChartsPanel::on_menu_delete_all_plots(wxCommandEvent& event) {
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
    // skip event
    event.Skip();
}

void ChartsPanel::on_menu_step_tool(wxCommandEvent& event) {
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
    // skip event
    event.Skip();
}

void ChartsPanel::on_menu_add_chart(wxCommandEvent& event) {
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
    // skip event
    event.Skip();
}

void ChartsPanel::on_menu_delete_chart(wxCommandEvent& event) {
    // check we have a selected chart
    if (m_selected_chart != nullptr) {
        // log information
        spdlog::debug("User requested deleting the new chart at index: {}", m_selected_chart_index);
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
    // skip event
    event.Skip();
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

void ChartsPanel::update(ExpressionManager& expression_manager, const StepInformation& step_information, const std::string& abscissa_label, const AbscissaScale abscissa_scale, const size_t decimate_target) {
    // update fields
    m_expression_manager = &expression_manager;
    m_step_information = &step_information;
    m_abscissa_label = abscissa_label;
    m_abscissa_scale = abscissa_scale;
    m_decimate_target = decimate_target;
    // check charts are present, if not add one
    if (!m_charts.empty()) {
        // loop charts
        for (const auto& chart : m_charts) {
            // update chart with new information
            chart->update(m_expression_manager, m_step_information, m_abscissa_label, m_abscissa_scale);
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

void ChartsPanel::refresh_charts(int frames) { m_render_chart_frames = frames; }
