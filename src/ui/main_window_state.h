#pragma once

#include <string_view>

// application states of the main window
enum class AppState
{
    Empty,
    Editing,
    Running,
    Chart,
    NetlistWithResults,
    ChartOnly
};

// input flags describing the current main window state
struct ActionStateInput
{
    bool has_netlist = false;
    bool has_raw = false;
    bool charts_shown = false;
    bool simulation_running = false;
    bool netlist_editor_dirty = false;
    bool has_netlist_file = false;
    bool output_hidden = false;
    bool log_has_content = false;
};

// enablement of each main window action
struct ActionStateEnablement
{
    bool open = false;
    bool save = false;
    bool show_netlist = false;
    bool show_charts = false;
    bool show_simulation_output = false;
    bool configure_simulation = false;
    bool run_simulation = false;
};

// derive the application state from the input flags
AppState derive_app_state(const ActionStateInput& input);

// compute action enablement from the input flags
ActionStateEnablement compute_action_enablement(const ActionStateInput& input);

// human-readable name of an application state
std::string_view app_state_name(AppState state);
