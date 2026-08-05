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

} // namespace

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

SimulationParametersDialog::SimulationParametersDialog(wxWindow* parent, const SimulationConfig& config) :
    wxDialog(parent, wxID_ANY, "Xyce Simulation", wxDefaultPosition, wxSize(960, 720), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
    // main vertical sizer for the entire dialog
    auto* main_sizer = new wxBoxSizer(wxVERTICAL);

    // --- top area: TabbedPanel (sidebar + content pages) ---
    m_tabbed_panel = new TabbedPanel(this);
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

    main_sizer->Add(m_tabbed_panel, 1, wxEXPAND);

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
    Refresh();
}

wxPanel* SimulationParametersDialog::create_transient_parameters_panel(wxWindow* parent) {
    // page
    auto* page = new wxPanel(parent);
    // resizer
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    // simuation panel
    m_tran_panel = new TransientParametersPanel(parent);
    m_tran_panel->Reparent(page);
    sizer->Add(m_tran_panel, 1, wxEXPAND);
    // sensitivity section for TRAN
    m_tran_sensitivity = new SensitivitySectionPanel(page);
    sizer->Add(m_tran_sensitivity, 0, wxEXPAND | wxTOP, FromDIP(8));
    // step parameters section for this tab
    m_tran_step_params = new StepParametersPanel(page);
    sizer->Add(m_tran_step_params, 0, wxEXPAND | wxTOP, FromDIP(8));
    // set resizer for the page
    page->SetSizer(sizer);
    // exit
    return page;
}

wxPanel* SimulationParametersDialog::create_op_parameters_panel(wxWindow* parent) {
    // page
    auto* page = new wxPanel(parent);
    // resizer
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    // op parameters panel
    m_op_panel = new OpParametersPanel(parent);
    m_op_panel->Reparent(page);
    sizer->Add(m_op_panel, 1, wxEXPAND);
    // step parameters section for this tab
    m_op_step_params = new StepParametersPanel(page);
    sizer->Add(m_op_step_params, 0, wxEXPAND | wxTOP, FromDIP(8));
    // set resizer for the page
    page->SetSizer(sizer);
    // exit
    return page;
}

wxPanel* SimulationParametersDialog::create_dc_parameters_panel(wxWindow* parent) {
    // page
    auto* page = new wxPanel(parent);
    // resizer
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    // dc parameters panel
    m_dc_panel = new DcParametersPanel(parent);
    m_dc_panel->Reparent(page);
    sizer->Add(m_dc_panel, 1, wxEXPAND);
    // sensitivity section for DC
    m_dc_sensitivity = new SensitivitySectionPanel(page);
    sizer->Add(m_dc_sensitivity, 0, wxEXPAND | wxTOP, FromDIP(8));
    // step parameters section for this tab
    m_dc_step_params = new StepParametersPanel(page);
    sizer->Add(m_dc_step_params, 0, wxEXPAND | wxTOP, FromDIP(8));
    // set resizer for the page
    page->SetSizer(sizer);
    // exit
    return page;
}

wxPanel* SimulationParametersDialog::create_ac_parameters_panel(wxWindow* parent) {
    // page
    auto* page = new wxPanel(parent);
    // resizer
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    // ac parameters panel
    m_ac_panel = new AcParametersPanel(parent);
    m_ac_panel->Reparent(page);
    sizer->Add(m_ac_panel, 1, wxEXPAND);
    // sensitivity section for AC
    m_ac_sensitivity = new SensitivitySectionPanel(page);
    sizer->Add(m_ac_sensitivity, 0, wxEXPAND | wxTOP, FromDIP(8));
    // step parameters section for this tab
    m_ac_step_params = new StepParametersPanel(page);
    sizer->Add(m_ac_step_params, 0, wxEXPAND | wxTOP, FromDIP(8));
    // set resizer for the page
    page->SetSizer(sizer);
    // exit
    return page;
}

wxPanel* SimulationParametersDialog::create_noise_parameters_panel(wxWindow* parent) {
    // page
    auto* page = new wxPanel(parent);
    // resizer
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    // noise parameters panel
    m_noise_panel = new NoiseParametersPanel(parent);
    m_noise_panel->Reparent(page);
    sizer->Add(m_noise_panel, 1, wxEXPAND);
    // step parameters section for this tab
    m_noise_step_params = new StepParametersPanel(page);
    sizer->Add(m_noise_step_params, 0, wxEXPAND | wxTOP, FromDIP(8));
    // set resizer for the page
    page->SetSizer(sizer);
    // exit
    return page;
}

wxPanel* SimulationParametersDialog::create_hb_parameters_panel(wxWindow* parent) {
    // page
    auto* page = new wxPanel(parent);
    // resizer
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    // hb parameters panel
    m_hb_panel = new HbParametersPanel(parent);
    m_hb_panel->Reparent(page);
    sizer->Add(m_hb_panel, 1, wxEXPAND);
    // step parameters section for this tab
    m_hb_step_params = new StepParametersPanel(page);
    sizer->Add(m_hb_step_params, 0, wxEXPAND | wxTOP, FromDIP(8));
    // set resizer for the page
    page->SetSizer(sizer);
    // exit
    return page;
}

wxPanel* SimulationParametersDialog::create_lin_parameters_panel(wxWindow* parent) {
    // page
    auto* page = new wxPanel(parent);
    // resizer
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    // lin parameters panel
    m_lin_panel = new LinParametersPanel(parent);
    m_lin_panel->Reparent(page);
    sizer->Add(m_lin_panel, 1, wxEXPAND);
    // step parameters section for this tab
    m_lin_step_params = new StepParametersPanel(page);
    sizer->Add(m_lin_step_params, 0, wxEXPAND | wxTOP, FromDIP(8));
    // set resizer for the page
    page->SetSizer(sizer);
    // exit
    return page;
}

void SimulationParametersDialog::on_page_changed(wxCommandEvent&) {
    int page = m_tabbed_panel->get_selection();
    m_analysis_type = std::string(PAGE_ANALYSIS_TYPES[page].ToUTF8());
    Layout();
}

SimulationConfig SimulationParametersDialog::build_preview_config() const {
    int page = m_tabbed_panel->get_selection();
    std::string analysis_type = std::string(PAGE_ANALYSIS_TYPES[page].ToUTF8());
    std::variant<std::monostate, AcSimulationParameters, DCSimulationParameters, HbSimulationParameters, LinSimulationParameters, NoiseSimulationParameters, OpSimulationParameters, TransientSimulationParameters> analysis = std::monostate{};
    // collect step parameters from the currently selected tab
    std::vector<StepParameters> steps;

    switch (page) {
    case PAGE_OP:
        analysis = m_op_panel->build_op_parameters();
        {
            auto step = m_op_step_params->build_step_parameters();
            if (step.enabled)
                steps.push_back(std::move(step));
        }
        break;
    case PAGE_TRAN: {
        auto params = m_tran_panel->build_transient_parameters();
        auto sens = m_tran_sensitivity->build_sens_parameter("TRAN");
        params.sensitivity = std::move(sens);
        analysis = std::move(params);
        auto step = m_tran_step_params->build_step_parameters();
        if (step.enabled)
            steps.push_back(std::move(step));
        break;
    }
    case PAGE_DC: {
        auto params = m_dc_panel->build_dc_parameters();
        auto sens = m_dc_sensitivity->build_sens_parameter("DC");
        params.sensitivity = std::move(sens);
        analysis = std::move(params);
        auto step = m_dc_step_params->build_step_parameters();
        if (step.enabled)
            steps.push_back(std::move(step));
        break;
    }
    case PAGE_AC: {
        auto params = m_ac_panel->build_ac_parameters();
        auto sens = m_ac_sensitivity->build_sens_parameter("AC");
        params.sensitivity = std::move(sens);
        analysis = std::move(params);
        auto step = m_ac_step_params->build_step_parameters();
        if (step.enabled)
            steps.push_back(std::move(step));
        break;
    }
    case PAGE_NOISE:
        analysis = m_noise_panel->build_noise_parameters();
        {
            auto step = m_noise_step_params->build_step_parameters();
            if (step.enabled)
                steps.push_back(std::move(step));
        }
        break;
    case PAGE_HB:
        analysis = m_hb_panel->build_hb_parameters();
        {
            auto step = m_hb_step_params->build_step_parameters();
            if (step.enabled)
                steps.push_back(std::move(step));
        }
        break;
    case PAGE_LIN:
        analysis = m_lin_panel->build_lin_parameters();
        {
            auto step = m_lin_step_params->build_step_parameters();
            if (step.enabled)
                steps.push_back(std::move(step));
        }
        break;
    }

    GlobalSettingsPanel* global_settings = nullptr;
    switch (page) {
    case PAGE_OP:
        global_settings = m_op_panel->get_global_settings();
        break;
    case PAGE_TRAN:
        global_settings = m_tran_panel->get_global_settings();
        break;
    case PAGE_DC:
        global_settings = m_dc_panel->get_global_settings();
        break;
    case PAGE_AC:
        global_settings = m_ac_panel->get_global_settings();
        break;
    case PAGE_NOISE:
        global_settings = m_noise_panel->get_global_settings();
        break;
    case PAGE_HB:
        global_settings = m_hb_panel->get_global_settings();
        break;
    case PAGE_LIN:
        global_settings = m_lin_panel->get_global_settings();
        break;
    }
    // read replace ground from the active page's global settings panel
    bool replace_ground = true;
    if (global_settings != nullptr)
        replace_ground = global_settings->get_replace_ground();
    // exit
    return SimulationConfig(std::move(analysis_type), std::move(analysis), std::move(steps), m_data_blocks, OptionParameters({}, {}, {}, {}, {}), m_unassociated_prints, replace_ground);
}

void SimulationParametersDialog::apply_config(const SimulationConfig& config) {
    // reset panels to defaults
    m_op_panel->apply(OpSimulationParameters(false, false, false, {}, "", "", false, "", "", {}, {}, std::nullopt));
    m_tran_panel->apply(TransientSimulationParameters("", "", "", "", "", {}, std::nullopt, {}, {}, {}, std::nullopt));
    m_dc_panel->apply(DCSimulationParameters("", "", "", "", "", "", {}, "", "", "", "", "", "", std::nullopt, {}, std::nullopt));
    m_ac_panel->apply(AcSimulationParameters("", "", "", "", "", std::nullopt, {}, std::nullopt));
    m_noise_panel->apply(NoiseSimulationParameters("", "", "", "", "", "", "", {}, "", std::nullopt));
    m_hb_panel->apply(HbSimulationParameters({}, {}, std::nullopt, std::nullopt, std::nullopt, std::nullopt, {}, {}));
    m_lin_panel->apply(LinSimulationParameters(false, "", "", "", "", "", "", "", "", "", "", "", std::nullopt));

    // sync replace ground to all pages
    m_op_panel->get_global_settings()->set_replace_ground(config.replace_ground);
    m_tran_panel->get_global_settings()->set_replace_ground(config.replace_ground);
    m_dc_panel->get_global_settings()->set_replace_ground(config.replace_ground);
    m_ac_panel->get_global_settings()->set_replace_ground(config.replace_ground);
    m_noise_panel->get_global_settings()->set_replace_ground(config.replace_ground);
    m_hb_panel->get_global_settings()->set_replace_ground(config.replace_ground);
    m_lin_panel->get_global_settings()->set_replace_ground(config.replace_ground);

    // reset per-tab sensitivity sections
    m_tran_sensitivity->apply(nullptr);
    m_dc_sensitivity->apply(nullptr);
    m_ac_sensitivity->apply(nullptr);

    // reset per-tab step parameters
    m_op_step_params->apply(StepParameters());
    m_tran_step_params->apply(StepParameters());
    m_dc_step_params->apply(StepParameters());
    m_ac_step_params->apply(StepParameters());
    m_noise_step_params->apply(StepParameters());
    m_hb_step_params->apply(StepParameters());
    m_lin_step_params->apply(StepParameters());

    // apply the matching analysis type
    if (std::holds_alternative<OpSimulationParameters>(config.analysis)) {
        m_op_panel->apply(std::get<OpSimulationParameters>(config.analysis));
        if (!config.steps.empty())
            m_op_step_params->apply(config.steps[0]);
    }
    else if (std::holds_alternative<TransientSimulationParameters>(config.analysis)) {
        const auto& params = std::get<TransientSimulationParameters>(config.analysis);
        m_tran_panel->apply(params);
        if (params.sensitivity) {
            m_tran_sensitivity->apply(&*params.sensitivity);
        }
        if (!config.steps.empty())
            m_tran_step_params->apply(config.steps[0]);
    }
    else if (std::holds_alternative<DCSimulationParameters>(config.analysis)) {
        const auto& params = std::get<DCSimulationParameters>(config.analysis);
        m_dc_panel->apply(params);
        if (params.sensitivity) {
            m_dc_sensitivity->apply(&*params.sensitivity);
        }
        if (!config.steps.empty())
            m_dc_step_params->apply(config.steps[0]);
    }
    else if (std::holds_alternative<AcSimulationParameters>(config.analysis)) {
        const auto& params = std::get<AcSimulationParameters>(config.analysis);
        m_ac_panel->apply(params);
        if (params.sensitivity) {
            m_ac_sensitivity->apply(&*params.sensitivity);
        }
        if (!config.steps.empty())
            m_ac_step_params->apply(config.steps[0]);
    }
    else if (std::holds_alternative<NoiseSimulationParameters>(config.analysis)) {
        m_noise_panel->apply(std::get<NoiseSimulationParameters>(config.analysis));
        if (!config.steps.empty())
            m_noise_step_params->apply(config.steps[0]);
    }
    else if (std::holds_alternative<HbSimulationParameters>(config.analysis)) {
        m_hb_panel->apply(std::get<HbSimulationParameters>(config.analysis));
        if (!config.steps.empty())
            m_hb_step_params->apply(config.steps[0]);
    }
    else if (std::holds_alternative<LinSimulationParameters>(config.analysis)) {
        m_lin_panel->apply(std::get<LinSimulationParameters>(config.analysis));
        if (!config.steps.empty())
            m_lin_step_params->apply(config.steps[0]);
    }

    // store data blocks
    m_data_blocks = config.data_blocks;
    // store unassociated print directives (no UI, preserved verbatim for round-trip)
    m_unassociated_prints = config.unassociated_prints;
}

SimulationConfig SimulationParametersDialog::get_config() const { return build_preview_config(); }
