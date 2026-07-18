#include <imgui.h>
#include <spdlog/spdlog.h>
#include <wx/window.h>

#include "charts_panel.h"

ChartsPanel::ChartsPanel(wxWindow* parent, const wxWindowID id)
    : wxPanel(parent, id, wxDefaultPosition, wxDefaultSize, wxNO_BORDER | wxFULL_REPAINT_ON_RESIZE) {
    // bind event handlers
    Bind(wxEVT_PAINT, &ChartsPanel::on_paint, this);
    Bind(wxEVT_SIZE, &ChartsPanel::on_size, this);
    Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) { return; });
}

ChartsPanel::~ChartsPanel() {
    // terminate
    terminate();
}

void ChartsPanel::on_size(wxSizeEvent& event) {
    // check we have initialized
    if (m_charts_panel && m_metal_layer)
        update_bounds();
    // skip even
    event.Skip();
}

void ChartsPanel::on_paint(wxPaintEvent&) {
    spdlog::info("paint");
    // required call
    wxPaintDC dc(this);
    // initialize widget (only the first time)
    initialize();
}

Chart& ChartsPanel::add_chart(ExpressionManager& expression_manager, const StepInformation& step_information, Expression<double>& abscissa, const std::string& abscissa_label, AbscissaScale abscissa_scale, size_t decimate_target) {
    // create chart instance, append to vector
    return m_charts.emplace_back(expression_manager, step_information, abscissa, abscissa_label, abscissa_scale, decimate_target);
}

void ChartsPanel::delete_all_charts() {
    // simple, clean vector
    m_charts.clear();
    // refresh visualization
    Refresh(false);
}

void ChartsPanel::refresh_charts() {
    // render
    render_frame([this]()-> void {
        spdlog::info("Charts Panel render");
        // panel
        if (ImGui::Begin("Charts Panel", nullptr, ImGuiWindowFlags_NoTitleBar)) {
            // check we have charts to render
            if (!m_charts.empty()) {
                // available are
                const ImVec2 total_space = ImGui::GetContentRegionAvail();
                // chart height
                const double height = total_space.y / m_charts.size();
                // render charts within native frame
                for (size_t i = 0; i < m_charts.size(); ++i) {
                    // area name
                    auto name = std::format("Chart {}", i);
                    // create child with given height, use the whole area in the horizontal
                    if (ImGui::BeginChild(name.c_str(), ImVec2(0, height), true)) {
                        // render chart
                        m_charts[i].render();
                        // close
                        ImGui::EndChild();
                    }
                    // move cursor back
                    ImGui::SameLine();
                }
            }
            // close
            ImGui::End();
        }
    });
}