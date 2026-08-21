#pragma once

#include <functional>
#include <memory>

#include <slint.h>

#include "../main_window_view_def.h"
#include "charts_renderer.h"

namespace fft_dialog_view
{
    // owns the slint FFT setup dialog window; shown on demand for a chart and
    // delivers the accepted expressions and FFT parameters to the presenter,
    // which runs the transform and spawns a result window. The dialog must be
    // kept alive for the lifetime of the viewer, so it lives in its own
    // translation unit (the generated dialog header defines a SharedGlobals
    // type that conflicts with the main window header in the same compilation
    // unit).
    class FftDialogView
    {
    public:
        explicit FftDialogView(ChartsRenderer& renderer);

        FftDialogView(const FftDialogView&) = delete;
        FftDialogView& operator=(const FftDialogView&) = delete;

        ~FftDialogView();

        // the underlying slint window of the dialog; the dialog must be shown
        // before accessing it
        slint::Window& window();

        // shows the dialog for the chart at the given index, pre-selecting the
        // chart's plotted expressions. The on_closed callback is invoked on
        // both accept and cancel, after the dialog window is hidden, so the
        // caller can release the modal state; on accept the selected expressions
        // and FFT parameters are delivered through the event handler.
        void show_for_chart(size_t chart_index, MainWindowViewDefEvents& handler, const std::function<void()>& on_closed);

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
} // namespace fft_dialog_view
