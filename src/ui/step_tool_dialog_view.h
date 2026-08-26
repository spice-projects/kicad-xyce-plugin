#pragma once

#include <functional>
#include <memory>

#include <slint.h>

#include "charts_renderer.h"

namespace step_tool_dialog_view
{
    // owns the slint step tool dialog window; shown on demand for a chart and
    // applies the accepted step selection back to the chart through the renderer.
    // The dialog must be kept alive for the lifetime of the viewer, so it lives
    // in its own translation unit (the generated dialog header defines a
    // SharedGlobals type that conflicts with the main window header in the same
    // compilation unit).
    class StepToolDialogView
    {
    public:
        explicit StepToolDialogView(ChartsRenderer& renderer);

        StepToolDialogView(const StepToolDialogView&) = delete;
        StepToolDialogView& operator=(const StepToolDialogView&) = delete;

        ~StepToolDialogView();

        // the underlying slint window of the dialog; the dialog must be shown
        // before accessing it
        slint::Window& window();

        // shows the dialog for the chart at the given index, pre-checking the
        // chart's selected steps. The on_closed callback is invoked on both
        // accept and cancel, after the dialog window is hidden, so the caller
        // can release the modal state.
        void show_for_chart(size_t chart_index, const std::function<void()>& on_closed);

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
} // namespace step_tool_dialog_view
