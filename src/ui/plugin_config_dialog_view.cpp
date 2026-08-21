#include <optional>
#include <string>

#include <slint.h>

#include <plugin_config_dialog.h>

#include "../config/plugin_config.h"
#include "file_dialog.h"
#include "main_window_view_def.h"
#include "plugin_config_dialog_view.h"

namespace plugin_config_dialog_view
{
    namespace
    {
        // validation feedback strings, mirroring the wx dialog
        constexpr const char* REQUIRED_ERROR = "Xyce executable path is required";
        constexpr const char* INVALID_ERROR = "Selected path is not an executable file";
    } // namespace

    struct PluginConfigDialogView::Impl
    {
        // the slint dialog window, created lazily on the first use
        slint::ComponentHandle<plugin_config_dialog::PluginConfigDialog> dialog;

        // presenter notified with the accepted configuration
        MainWindowViewDefEvents* handler = nullptr;

        // notified on both accept and cancel, after the dialog window is hidden;
        // the caller releases the modal state from here
        std::function<void()> on_closed;

        Impl() :
            dialog(plugin_config_dialog::PluginConfigDialog::create()) {
            // wire callbacks
            dialog->on_browse_clicked([this] { browse(); });
            dialog->on_accepted([this] { accept(); });
            dialog->on_dismissed([this] { dismiss(); });
        }

        void browse() {
            // open the native file picker for the Xyce executable
            const auto path = FileDialog::open_xyce_executable();
            if (!path.has_value())
                return;
            // push the selected path into the field
            dialog->set_xyce_path(slint::SharedString(path->string()));
            // clear any previous validation message after a new selection
            dialog->set_show_error(false);
        }

        void accept() {
            // read the path and trim surrounding whitespace
            const std::string raw = std::string(dialog->get_xyce_path());
            // check non-empty path
            if (const auto first = raw.find_first_not_of(" \t\r\n"); first != std::string::npos) {
                // trim
                const auto last = raw.find_last_not_of(" \t\r\n");
                const PluginConfig config(raw.substr(first, last - first + 1));
                // reject path values that are not executable files
                if (!config.is_xyce_executable_valid()) {
                    // show error message
                    dialog->set_error_message(slint::SharedString(INVALID_ERROR));
                    dialog->set_show_error(true);
                    // exit
                    return;
                }
                // persist the validated configuration
                config.save();
                // close the dialog before delivering the result
                dialog->hide();
                // release the modal state held by the caller
                if (on_closed)
                    on_closed();
                // deliver the updated configuration to the presenter
                if (handler != nullptr)
                    handler->on_plugin_config_dialog_result(config);
                // exit
                return;
            }
            // require a non-empty path so the plugin can launch Xyce
            dialog->set_error_message(slint::SharedString(REQUIRED_ERROR));
            dialog->set_show_error(true);
        }

        void dismiss() {
            // hide the dialog and drop the pending state
            dialog->hide();
            // release the modal state held by the caller
            if (on_closed)
                on_closed();
        }
    };

    PluginConfigDialogView::PluginConfigDialogView() :
        m_impl(std::make_unique<Impl>()) {}

    PluginConfigDialogView::~PluginConfigDialogView() = default;

    slint::Window& PluginConfigDialogView::window() {
        // expose the dialog window (dialog must be shown first)
        return m_impl->dialog->window();
    }

    void PluginConfigDialogView::show(const PluginConfig& current, MainWindowViewDefEvents& handler, const std::function<void()>& on_closed) {
        // remember the result destination for the dialog lifetime
        m_impl->handler = &handler;
        // remember the close notification for this show
        m_impl->on_closed = on_closed;
        // seed the path field with the current configuration
        m_impl->dialog->set_xyce_path(slint::SharedString(current.xyce_executable_path()));
        // clear any previous validation feedback
        m_impl->dialog->set_show_error(false);
        // show the dialog window
        m_impl->dialog->show();
    }
} // namespace plugin_config_dialog_view
