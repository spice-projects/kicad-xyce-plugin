#include "charts_panel.h"

void ChartsPanel::initialize() {
    // check flag
    if (m_charts_panel)
        return;
}

void ChartsPanel::terminate() {
    // check flag
    if (!m_charts_panel)
        return;
    // update flag
    m_charts_panel = nullptr;
}

void ChartsPanel::render_frame(const std::function<void()>& renderer) {
}

void ChartsPanel::update_bounds() {
}
