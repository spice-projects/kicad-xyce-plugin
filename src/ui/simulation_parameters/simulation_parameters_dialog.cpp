#include <string>
#include <vector>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/arrstr.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/settings.h>
#include <wx/simplebook.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/tglbtn.h>
#include <wx/tokenzr.h>
#endif

#include "ac_parameters_panel.h"
#include "dc_parameters_panel.h"
#include "hb_parameters_panel.h"
#include "lin_parameters_panel.h"
#include "noise_parameters_panel.h"
#include "op_parameters_panel.h"
#include "sensitivity_section_panel.h"
#include "simulation_parameters/ac_simulation_parameters.h"
#include "simulation_parameters/dc_simulation_parameters.h"
#include "simulation_parameters/hb_simulation_parameters.h"
#include "simulation_parameters/lin_simulation_parameters.h"
#include "simulation_parameters/noise_simulation_parameters.h"
#include "simulation_parameters/op_simulation_parameters.h"
#include "simulation_parameters/simulation_config.h"
#include "simulation_parameters/transient_simulation_parameters.h"
#include "simulation_parameters_dialog.h"
#include "transient_parameters_panel.h"

class TabbedPanel : public wxPanel
{
public:
    explicit TabbedPanel(wxWindow* parent) :
        wxPanel(parent) {
        // main horizontal layout: sidebar on the left, content area on the right
        m_main_sizer = new wxBoxSizer(wxHORIZONTAL);
        // vertical sidebar holding toggle buttons for each tab
        m_sidebar_sizer = new wxBoxSizer(wxVERTICAL);
        m_main_sizer->Add(m_sidebar_sizer, 0, wxEXPAND | wxLEFT | wxTOP | wxBOTTOM, FromDIP(8));
        // top padding above the first toggle button
        m_sidebar_sizer->Add(0, FromDIP(8), 0, 0);
        // content area that shows the selected page
        m_simplebook = new wxSimplebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
        m_main_sizer->Add(m_simplebook, 1, wxEXPAND | wxALL, FromDIP(8));
        // set the main sizer for this panel
        SetSizer(m_main_sizer);
    }

    wxToggleButton* add_tab(const wxString& label, wxWindow* page) {
        // sidebar toggle button for this tab
        auto* btn = new wxToggleButton(this, wxID_ANY, label, wxDefaultPosition, wxSize(FromDIP(72), FromDIP(36)));
        btn->SetMinSize(wxSize(FromDIP(72), FromDIP(36)));
        m_sidebar_sizer->Add(btn, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(4));
        // wrap the page in a scrollable container so content can scroll vertically when the dialog is too short
        auto* scroll = new wxScrolledWindow(m_simplebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxBORDER_NONE);
        scroll->SetScrollRate(0, FromDIP(10));
        auto* scroll_sizer = new wxBoxSizer(wxVERTICAL);
        page->Reparent(scroll);
        // add the page to the scrollable container and let it expand to fill the available width
        scroll_sizer->Add(page, 1, wxEXPAND);
        scroll->SetSizer(scroll_sizer);
        scroll->FitInside();
        // register the button and page at the current index
        size_t index = m_sidebar_buttons.size();
        m_sidebar_buttons.push_back(btn);
        m_simplebook->AddPage(scroll, label);
        // switch to this tab on click, deselect all others, and notify parent
        btn->Bind(wxEVT_TOGGLEBUTTON, [this, index](wxCommandEvent&) {
            // toggle the selected button on and all others off
            for (size_t j = 0; j < m_sidebar_buttons.size(); ++j)
                m_sidebar_buttons[j]->SetValue(j == index);
            // show the corresponding page
            m_simplebook->SetSelection(static_cast<int>(index));
            // create event and send to parent
            wxCommandEvent event(wxEVT_BOOKCTRL_PAGE_CHANGED, GetId());
            event.SetInt(static_cast<int>(index));
            event.SetEventObject(this);
            ProcessEvent(event);
        });
        return btn;
    }

    [[nodiscard]] int get_selection() const { return m_simplebook->GetSelection(); }

    void set_selection(size_t index) {
        // validate index and switch to the corresponding tab if valid
        if (index < m_sidebar_buttons.size()) {
            // toggle the selected button on and all others off
            for (size_t j = 0; j < m_sidebar_buttons.size(); ++j)
                m_sidebar_buttons[j]->SetValue(j == index);
            // show the corresponding page
            m_simplebook->SetSelection(static_cast<int>(index));
            // create event
            wxCommandEvent event(wxEVT_BOOKCTRL_PAGE_CHANGED, GetId());
            event.SetInt(static_cast<int>(index));
            event.SetEventObject(this);
            // send event to parent
            ProcessEvent(event);
        }
    }

private:
    wxBoxSizer* m_main_sizer = nullptr;
    wxBoxSizer* m_sidebar_sizer = nullptr;
    wxSimplebook* m_simplebook = nullptr;
    std::vector<wxToggleButton*> m_sidebar_buttons;
};

class SensitivityAwarePanel : public wxPanel
{
public:
    SensitivityAwarePanel(wxWindow* parent, wxWindow* inner_panel) :
        wxPanel(parent) {
        // vertical stack: inner panel on top, sensitivity section below
        auto* sizer = new wxBoxSizer(wxVERTICAL);
        // reparent the inner simulation panel into this wrapper
        inner_panel->Reparent(this);
        // let the inner panel grow to fill available horizontal space
        sizer->Add(inner_panel, 1, wxEXPAND);
        // sensitivity section sits below the inner panel
        m_sensitivity = new SensitivitySectionPanel(this);
        sizer->Add(m_sensitivity, 0, wxEXPAND | wxTOP, FromDIP(8));
        // attach the sizer
        SetSizer(sizer);
    }

    [[nodiscard]] SensitivitySectionPanel* get_sensitivity() const { return m_sensitivity; }

private:
    // embedded sensitivity controls for this tab
    SensitivitySectionPanel* m_sensitivity = nullptr;
};

namespace
{
    // page indices matching sidebar button order
    static constexpr int PAGE_OP = 0;
    static constexpr int PAGE_TRAN = 1;
    static constexpr int PAGE_DC = 2;
    static constexpr int PAGE_AC = 3;
    static constexpr int PAGE_NOISE = 4;
    static constexpr int PAGE_HB = 5;
    static constexpr int PAGE_LIN = 6;

    // analysis type strings for each page
    static const std::vector<wxString> PAGE_ANALYSIS_TYPES = {"OP", "TRAN", "DC", "AC", "NOISE", "HB", "LIN"};

    // sweep mode choices for step parameters
    static const std::vector<wxString> STEP_SWEEP_MODES = {"LIN", "DEC", "OCT", "LIST", "DATA"};

    // parse space-separated list values into strings
    [[nodiscard]] std::vector<std::string> parse_list_values(const wxString& text) {
        std::vector<std::string> values;
        wxString trimmed = wxString(text).Trim(true).Trim(false);
        if (trimmed.IsEmpty()) {
            return values;
        }
        wxStringTokenizer tokenizer(trimmed, " \t\r\n");
        while (tokenizer.HasMoreTokens()) {
            values.push_back(std::string(tokenizer.GetNextToken().ToUTF8()));
        }
        return values;
    }

    // format strings as space-separated text
    [[nodiscard]] wxString format_list_values(const std::vector<std::string>& items) {
        if (items.empty()) {
            return wxEmptyString;
        }
        wxString result;
        for (size_t i = 0; i < items.size(); ++i) {
            if (i > 0) {
                result += " ";
            }
            result += wxString::FromUTF8(items[i]);
        }
        return result;
    }
} // namespace

SimulationParametersDialog::SimulationParametersDialog(wxWindow* parent, const SimulationConfig& config) :
    wxDialog(parent, wxID_ANY, "Xyce Simulation", wxDefaultPosition, wxSize(960, 720), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
    // main vertical sizer for the entire dialog
    auto* main_sizer = new wxBoxSizer(wxVERTICAL);

    // scrollable container for the sidebar/book, sensitivity section, and step
    // parameters box so their content stays reachable even when the dialog is
    // resized smaller than the combined content height
    m_scroll_window = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxBORDER_NONE);
    m_scroll_window->SetScrollRate(0, FromDIP(10));
    auto* scroll_sizer = new wxBoxSizer(wxVERTICAL);

    // --- top area: TabbedPanel (sidebar + content pages) ---
    m_tabbed_panel = new TabbedPanel(m_scroll_window);
    // .op
    m_tabbed_panel->add_tab(".OP", create_op_parameters_panel(m_tabbed_panel));
    // .tran
    m_tabbed_panel->add_tab(".TRAN", create_transient_parameters_panel(m_tabbed_panel));
    // .dc
    m_tabbed_panel->add_tab(".DC", create_dc_parameters_panel(m_tabbed_panel));
    // .ac
    m_tabbed_panel->add_tab(".AC", create_ac_parameters_panel(m_tabbed_panel));
    // .noise
    m_tabbed_panel->add_tab(".NOISE", create_noise_parameters_panel(m_tabbed_panel));
    // .hb
    m_tabbed_panel->add_tab(".HB", create_hb_parameters_panel(m_tabbed_panel));
    // .lin
    m_tabbed_panel->add_tab(".LIN", create_lin_parameters_panel(m_tabbed_panel));

    // notify the dialog when the page changes
    m_tabbed_panel->Bind(wxEVT_BOOKCTRL_PAGE_CHANGED, [this](wxCommandEvent& evt) { on_page_changed(evt); });

    scroll_sizer->Add(m_tabbed_panel, 1, wxEXPAND);

    // --- step parameters section ---
    auto* step_box = new wxStaticBoxSizer(wxVERTICAL, m_scroll_window, "Step Parameters (.STEP)");
    auto* step_panel = step_box->GetStaticBox();
    auto* step_sizer = new wxBoxSizer(wxVERTICAL);

    // enable checkbox
    m_step_enable_cb = new wxCheckBox(step_panel, wxID_ANY, "Enable step sweep");
    step_sizer->Add(m_step_enable_cb, 0, wxBOTTOM, FromDIP(4));

    // step field grid: 2 columns (label | control)
    auto* step_grid = new wxFlexGridSizer(2, FromDIP(8), FromDIP(8));
    step_grid->AddGrowableCol(1, 1);

    // sweep mode row
    auto* mode_label = new wxStaticText(step_panel, wxID_ANY, "Sweep mode");
    step_grid->Add(mode_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    wxArrayString mode_choices;
    for (const auto& mode : STEP_SWEEP_MODES) {
        mode_choices.Add(mode);
    }
    m_step_sweep_mode_choice = new wxChoice(step_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, mode_choices);
    m_step_sweep_mode_choice->SetSelection(0);
    step_grid->Add(m_step_sweep_mode_choice, 0, wxEXPAND, 0);

    // variable row
    auto* var_label = new wxStaticText(step_panel, wxID_ANY, "Variable");
    step_grid->Add(var_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_step_variable_text = new wxTextCtrl(step_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    m_step_variable_text->SetHint("e.g. TEMP");
    step_grid->Add(m_step_variable_text, 0, wxEXPAND, 0);

    // start row
    auto* start_label = new wxStaticText(step_panel, wxID_ANY, "Start");
    step_grid->Add(start_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_step_start_text = new wxTextCtrl(step_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    step_grid->Add(m_step_start_text, 0, wxEXPAND, 0);

    // stop row
    auto* stop_label = new wxStaticText(step_panel, wxID_ANY, "Stop");
    step_grid->Add(stop_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_step_stop_text = new wxTextCtrl(step_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    step_grid->Add(m_step_stop_text, 0, wxEXPAND, 0);

    // step row
    auto* step_label = new wxStaticText(step_panel, wxID_ANY, "Step");
    step_grid->Add(step_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_step_step_text = new wxTextCtrl(step_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    step_grid->Add(m_step_step_text, 0, wxEXPAND, 0);

    // points row
    auto* points_label = new wxStaticText(step_panel, wxID_ANY, "Points");
    step_grid->Add(points_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_step_points_text = new wxTextCtrl(step_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    step_grid->Add(m_step_points_text, 0, wxEXPAND, 0);

    // list values row
    auto* list_vals_label = new wxStaticText(step_panel, wxID_ANY, "List values");
    step_grid->Add(list_vals_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_step_list_values_text = new wxTextCtrl(step_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    m_step_list_values_text->SetHint("space-separated");
    step_grid->Add(m_step_list_values_text, 0, wxEXPAND, 0);

    // data table row
    auto* data_table_label = new wxStaticText(step_panel, wxID_ANY, "Data table");
    step_grid->Add(data_table_label, 0, wxALIGN_CENTER_VERTICAL, 0);
    m_step_data_table_text = new wxTextCtrl(step_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
    step_grid->Add(m_step_data_table_text, 0, wxEXPAND, 0);

    step_sizer->Add(step_grid, 0, wxEXPAND | wxLEFT, FromDIP(16));
    step_box->Add(step_sizer, 0, wxEXPAND | wxALL, FromDIP(8));

    scroll_sizer->Add(step_box, 0, wxEXPAND | wxRIGHT | wxBOTTOM, FromDIP(8));

    // attach the scroll sizer and add the scroll window to the main layout;
    // it grows to fill available space and scrolls when content overflows
    m_scroll_window->SetSizer(scroll_sizer);
    main_sizer->Add(m_scroll_window, 1, wxEXPAND);

    // error label + buttons
    auto* footer_sizer = new wxBoxSizer(wxHORIZONTAL);

    m_error_label = new wxStaticText(this, wxID_ANY, wxEmptyString);
    m_error_label->SetForegroundColour(wxColour("#CC0000"));
    footer_sizer->Add(m_error_label, 1, wxALIGN_CENTER_VERTICAL, 0);

    auto* button_sizer = CreateStdDialogButtonSizer(wxAPPLY | wxCANCEL);
    Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { EndModal(wxID_OK); }, wxID_APPLY);
    footer_sizer->Add(button_sizer, 0, wxLEFT, FromDIP(8));

    main_sizer->Add(footer_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(8));

    SetSizer(main_sizer);

    // apply the initial config
    apply_config(config);

    // select the appropriate initial page
    int initial_page = 0;
    if (config.analysis_type == "TRAN") {
        initial_page = PAGE_TRAN;
    }
    else if (config.analysis_type == "DC") {
        initial_page = PAGE_DC;
    }
    else if (config.analysis_type == "AC") {
        initial_page = PAGE_AC;
    }
    else if (config.analysis_type == "NOISE") {
        initial_page = PAGE_NOISE;
    }
    else if (config.analysis_type == "HB") {
        initial_page = PAGE_HB;
    }
    else if (config.analysis_type == "LIN") {
        initial_page = PAGE_LIN;
    }

    m_tabbed_panel->set_selection(initial_page);

    Layout();
    m_scroll_window->FitInside();
    // re-layout the scroll window's content now that FitInside() may have
    // shown/resized the vertical scrollbar; otherwise content is sized for
    // the pre-scrollbar width and overruns into the scrollbar on the right
    m_scroll_window->Layout();
    Refresh();
}

wxPanel* SimulationParametersDialog::create_transient_parameters_panel(wxWindow* parent) {
    m_tran_panel = new TransientParametersPanel(parent);
    auto* wrapper = new SensitivityAwarePanel(parent, m_tran_panel);
    m_tran_sensitivity = wrapper->get_sensitivity();
    return wrapper;
}

wxPanel* SimulationParametersDialog::create_op_parameters_panel(wxWindow* parent) {
    // create panel
    m_op_panel = new OpParametersPanel(parent);
    // exit
    return m_op_panel;
}

wxPanel* SimulationParametersDialog::create_dc_parameters_panel(wxWindow* parent) {
    m_dc_panel = new DcParametersPanel(parent);
    auto* wrapper = new SensitivityAwarePanel(parent, m_dc_panel);
    m_dc_sensitivity = wrapper->get_sensitivity();
    return wrapper;
}

wxPanel* SimulationParametersDialog::create_ac_parameters_panel(wxWindow* parent) {
    m_ac_panel = new AcParametersPanel(parent);
    auto* wrapper = new SensitivityAwarePanel(parent, m_ac_panel);
    m_ac_sensitivity = wrapper->get_sensitivity();
    return wrapper;
}

wxPanel* SimulationParametersDialog::create_noise_parameters_panel(wxWindow* parent) {
    // create panel
    m_noise_panel = new NoiseParametersPanel(parent);
    // exit
    return m_noise_panel;
}

wxPanel* SimulationParametersDialog::create_hb_parameters_panel(wxWindow* parent) {
    // create panel
    m_hb_panel = new HbParametersPanel(parent);
    // exit
    return m_hb_panel;
}

wxPanel* SimulationParametersDialog::create_lin_parameters_panel(wxWindow* parent) {
    // create panel
    m_lin_panel = new LinParametersPanel(parent);
    // exit
    return m_lin_panel;
}

void SimulationParametersDialog::on_page_changed(wxCommandEvent&) {
    int page = m_tabbed_panel->get_selection();
    m_analysis_type = std::string(PAGE_ANALYSIS_TYPES[page].ToUTF8());

    m_scroll_window->Layout();
    // recompute the virtual (scrollable) size since different tabs may have
    // different content heights
    m_scroll_window->FitInside();
    // re-layout again since FitInside() may have shown/hidden the vertical
    // scrollbar, changing the width available to the content
    m_scroll_window->Layout();
    Layout();
}

SimulationConfig SimulationParametersDialog::build_preview_config() const {
    int page = m_tabbed_panel->get_selection();
    std::string analysis_type = std::string(PAGE_ANALYSIS_TYPES[page].ToUTF8());
    std::variant<std::monostate, AcSimulationParameters, DCSimulationParameters, HbSimulationParameters, LinSimulationParameters, NoiseSimulationParameters, OpSimulationParameters, TransientSimulationParameters> analysis = std::monostate{};

    switch (page) {
    case PAGE_OP:
        analysis = m_op_panel->build_op_parameters();
        break;
    case PAGE_TRAN: {
        auto params = m_tran_panel->build_transient_parameters();
        auto sens = m_tran_sensitivity->build_sens_parameter("TRAN");
        params.sensitivity = std::move(sens);
        analysis = std::move(params);
        break;
    }
    case PAGE_DC: {
        auto params = m_dc_panel->build_dc_parameters();
        auto sens = m_dc_sensitivity->build_sens_parameter("DC");
        params.sensitivity = std::move(sens);
        analysis = std::move(params);
        break;
    }
    case PAGE_AC: {
        auto params = m_ac_panel->build_ac_parameters();
        auto sens = m_ac_sensitivity->build_sens_parameter("AC");
        params.sensitivity = std::move(sens);
        analysis = std::move(params);
        break;
    }
    case PAGE_NOISE:
        analysis = m_noise_panel->build_noise_parameters();
        break;
    case PAGE_HB:
        analysis = m_hb_panel->build_hb_parameters();
        break;
    case PAGE_LIN:
        analysis = m_lin_panel->build_lin_parameters();
        break;
    }

    // read step parameters
    std::vector<StepParameters> steps;
    StepParameters current_step = read_step_parameters();
    if (current_step.enabled) {
        steps.push_back(std::move(current_step));
    }

    return SimulationConfig(std::move(analysis_type), std::move(analysis), std::move(steps), m_data_blocks, OptionParameters({}, {}, {}, {}), {});
}

StepParameters SimulationParametersDialog::read_step_parameters() const {
    bool enabled = m_step_enable_cb->GetValue();
    std::string sweep_mode;
    int sel = m_step_sweep_mode_choice->GetSelection();
    if (sel != wxNOT_FOUND && sel < static_cast<int>(STEP_SWEEP_MODES.size())) {
        sweep_mode = std::string(STEP_SWEEP_MODES[sel].ToUTF8());
    }
    std::string variable = std::string(m_step_variable_text->GetValue().ToUTF8());
    std::string start = std::string(m_step_start_text->GetValue().ToUTF8());
    std::string stop = std::string(m_step_stop_text->GetValue().ToUTF8());
    std::string step = std::string(m_step_step_text->GetValue().ToUTF8());
    std::string points = std::string(m_step_points_text->GetValue().ToUTF8());
    std::vector<std::string> list_values = parse_list_values(m_step_list_values_text->GetValue());
    std::string data_table_name = std::string(m_step_data_table_text->GetValue().ToUTF8());
    return StepParameters(std::move(sweep_mode), std::move(variable), std::move(start), std::move(stop), std::move(step), std::move(points), std::move(list_values), std::move(data_table_name), enabled);
}

void SimulationParametersDialog::apply_step_parameters(const StepParameters& params) {
    m_step_enable_cb->SetValue(params.enabled);

    // restore sweep mode
    int mode_index = m_step_sweep_mode_choice->FindString(wxString::FromUTF8(params.sweep_mode));
    if (mode_index != wxNOT_FOUND) {
        m_step_sweep_mode_choice->SetSelection(mode_index);
    }
    else {
        m_step_sweep_mode_choice->SetSelection(0);
    }

    m_step_variable_text->SetValue(wxString::FromUTF8(params.variable));
    m_step_start_text->SetValue(wxString::FromUTF8(params.start));
    m_step_stop_text->SetValue(wxString::FromUTF8(params.stop));
    m_step_step_text->SetValue(wxString::FromUTF8(params.step));
    m_step_points_text->SetValue(wxString::FromUTF8(params.points));
    m_step_list_values_text->SetValue(format_list_values(params.list_values));
    m_step_data_table_text->SetValue(wxString::FromUTF8(params.data_table_name));
}

void SimulationParametersDialog::apply_config(const SimulationConfig& config) {
    // reset panels to defaults
    m_op_panel->apply(OpSimulationParameters(false, false, false, {}, "", "", false, "", "", {}, {}, false, std::nullopt));
    m_tran_panel->apply(TransientSimulationParameters("", "", "", "", "", {}, false, std::nullopt, {}, {}, {}, std::nullopt));
    m_dc_panel->apply(DCSimulationParameters("", "", "", "", "", "", {}, "", "", "", "", "", "", false, std::nullopt, {}, std::nullopt));
    m_ac_panel->apply(AcSimulationParameters("", "", "", "", "", false, std::nullopt, {}, std::nullopt));
    m_noise_panel->apply(NoiseSimulationParameters("", "", "", "", "", "", "", {}, "", false, std::nullopt));
    m_hb_panel->apply(HbSimulationParameters({}, {}, std::nullopt, std::nullopt, std::nullopt, false, std::nullopt, {}, {}));
    m_lin_panel->apply(LinSimulationParameters(false, "", "", "", "", "", "", "", "", "", "", "", false, std::nullopt));

    // reset per-tab sensitivity sections
    m_tran_sensitivity->apply(nullptr);
    m_dc_sensitivity->apply(nullptr);
    m_ac_sensitivity->apply(nullptr);

    // apply the matching analysis type
    if (std::holds_alternative<OpSimulationParameters>(config.analysis)) {
        m_op_panel->apply(std::get<OpSimulationParameters>(config.analysis));
    }
    else if (std::holds_alternative<TransientSimulationParameters>(config.analysis)) {
        const auto& params = std::get<TransientSimulationParameters>(config.analysis);
        m_tran_panel->apply(params);
        if (params.sensitivity) {
            m_tran_sensitivity->apply(&*params.sensitivity);
        }
    }
    else if (std::holds_alternative<DCSimulationParameters>(config.analysis)) {
        const auto& params = std::get<DCSimulationParameters>(config.analysis);
        m_dc_panel->apply(params);
        if (params.sensitivity) {
            m_dc_sensitivity->apply(&*params.sensitivity);
        }
    }
    else if (std::holds_alternative<AcSimulationParameters>(config.analysis)) {
        const auto& params = std::get<AcSimulationParameters>(config.analysis);
        m_ac_panel->apply(params);
        if (params.sensitivity) {
            m_ac_sensitivity->apply(&*params.sensitivity);
        }
    }
    else if (std::holds_alternative<NoiseSimulationParameters>(config.analysis)) {
        m_noise_panel->apply(std::get<NoiseSimulationParameters>(config.analysis));
    }
    else if (std::holds_alternative<HbSimulationParameters>(config.analysis)) {
        m_hb_panel->apply(std::get<HbSimulationParameters>(config.analysis));
    }
    else if (std::holds_alternative<LinSimulationParameters>(config.analysis)) {
        m_lin_panel->apply(std::get<LinSimulationParameters>(config.analysis));
    }

    // apply step parameters (use first step or disabled default)
    if (!config.steps.empty()) {
        apply_step_parameters(config.steps[0]);
    }
    else {
        apply_step_parameters(StepParameters());
    }

    // store data blocks
    m_data_blocks = config.data_blocks;
}

SimulationConfig SimulationParametersDialog::get_config() const { return build_preview_config(); }
