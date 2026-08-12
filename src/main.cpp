// #include "app.h"
// #include <main-window.h>
#include <slint.h>
#include <add-plot-dialog.h>

// implement wxApp entry point
// wxIMPLEMENT_APP(App); // NOLINT

int main(int argc, char** argv) {

    // auto ui = MainWindow::create();
    // auto ui = FftDialog::create();
    auto ui = AddPlotDialog::create();

    // ui->on_request_increase_value([&] { ui->set_counter(ui->get_counter() + 1); });

    ui->show();

    slint::run_event_loop();
}
