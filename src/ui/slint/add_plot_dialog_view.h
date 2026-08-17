#pragma once

#include <functional>
#include <memory>

#include <slint.h>

#include "charts_renderer.h"

namespace add_plot_dialog_view
{
    // owns the slint add plot dialog window; shown on demand for a chart and
    // applies the accepted expression selection back to the renderer. The dialog
    // must be kept alive for the lifetime of the viewer, so it lives in its own
    // translation unit (the generated dialog header defines a SharedGlobals type
    // that conflicts with the main window header in the same compilation unit).
    class AddPlotDialogView
    {
    public:
        explicit AddPlotDialogView(ChartsRenderer& renderer);

        AddPlotDialogView(const AddPlotDialogView&) = delete;
        AddPlotDialogView& operator=(const AddPlotDialogView&) = delete;

        ~AddPlotDialogView();

        // the underlying slint window of the dialog; the dialog must be shown
        // before accessing it
        slint::Window& window();

        // shows the dialog for the chart at the given panel position [0, 1]. The
        // on_closed callback is invoked on both accept and cancel, after the
        // dialog window is hidden, so the caller can release the modal state.
        void show_for_chart(float chart_position, const std::function<void()>& on_closed);

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
} // namespace add_plot_dialog_view
