#pragma once

#include <functional>
#include <memory>

#include <slint.h>

#include "../../simulation_parameters/simulation_config.h"
#include "../main_window_view_def.h"

namespace simulation_parameters_dialog_view
{
    // owns the slint simulation parameters dialog window; seeded with the current
    // configuration and shown on demand. The dialog is non-modal, so the accepted
    // configuration is delivered to the presenter asynchronously through
    // MainWindowViewDefEvents. The dialog must be kept alive for the lifetime of
    // the viewer, so it lives in its own translation unit (the generated dialog
    // header defines a SharedGlobals type that conflicts with the main window
    // header in the same compilation unit).
    class SimulationParametersDialogView
    {
    public:
        SimulationParametersDialogView();

        SimulationParametersDialogView(const SimulationParametersDialogView&) = delete;
        SimulationParametersDialogView& operator=(const SimulationParametersDialogView&) = delete;

        ~SimulationParametersDialogView();

        // the underlying slint window of the dialog; the dialog must be shown
        // before accessing it
        slint::Window& window();

        // shows the dialog seeded with the current configuration; when the user
        // accepts, the selected configuration is delivered to the given handler
        // via on_simulation_parameters_dialog_result. The on_closed callback is
        // invoked on both accept and cancel, after the dialog window is hidden,
        // so the caller can release the modal state.
        void show(const SimulationConfig& current, MainWindowViewDefEvents& handler, const std::function<void()>& on_closed);

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
} // namespace simulation_parameters_dialog_view
