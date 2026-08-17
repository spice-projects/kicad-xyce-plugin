#pragma once

#include <functional>
#include <memory>

#include <slint.h>

#include "../../config/plugin_config.h"
#include "../main_window_view_def.h"

namespace plugin_config_dialog_view
{
    // owns the slint plugin configuration dialog window; seeded with the current
    // configuration and shown on demand. The dialog is non-modal, so the accepted
    // and validated configuration is persisted here and delivered to the presenter
    // asynchronously through MainWindowViewDefEvents. The dialog must be kept alive
    // for the lifetime of the viewer, so it lives in its own translation unit (the
    // generated dialog header defines a SharedGlobals type that conflicts with the
    // main window header in the same compilation unit).
    class PluginConfigDialogView
    {
    public:
        PluginConfigDialogView();

        PluginConfigDialogView(const PluginConfigDialogView&) = delete;
        PluginConfigDialogView& operator=(const PluginConfigDialogView&) = delete;

        ~PluginConfigDialogView();

        // the underlying slint window of the dialog; the dialog must be shown
        // before accessing it
        slint::Window& window();

        // shows the dialog seeded with the current configuration; when the user
        // accepts a valid path, the configuration is persisted and delivered to
        // the given handler via on_plugin_config_dialog_result. The on_closed
        // callback is invoked on both accept and cancel, after the dialog window
        // is hidden, so the caller can release the modal state.
        void show(const PluginConfig& current, MainWindowViewDefEvents& handler, const std::function<void()>& on_closed);

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };
} // namespace plugin_config_dialog_view
