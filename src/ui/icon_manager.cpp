#include <wx/wx.h>

#include "icon_data.h"
#include "icon_manager.h"

IconManager::IconManager() {
    // automatically generated icon bundle map
    m_bitmap_bundles["cancel"] = get_cancel_png_bundle();
    m_bitmap_bundles["cancel_dark"] = get_cancel_dark_png_bundle();
    m_bitmap_bundles["checked_ok"] = get_checked_ok_png_bundle();
    m_bitmap_bundles["checked_ok_dark"] = get_checked_ok_dark_png_bundle();
    m_bitmap_bundles["directory_open"] = get_directory_open_png_bundle();
    m_bitmap_bundles["directory_open_dark"] = get_directory_open_dark_png_bundle();
    m_bitmap_bundles["exit"] = get_exit_png_bundle();
    m_bitmap_bundles["exit_dark"] = get_exit_dark_png_bundle();
    m_bitmap_bundles["netlist"] = get_netlist_png_bundle();
    m_bitmap_bundles["netlist_dark"] = get_netlist_dark_png_bundle();
    m_bitmap_bundles["new_generic"] = get_new_generic_png_bundle();
    m_bitmap_bundles["new_generic_dark"] = get_new_generic_dark_png_bundle();
    m_bitmap_bundles["preference"] = get_preference_png_bundle();
    m_bitmap_bundles["preference_dark"] = get_preference_dark_png_bundle();
    m_bitmap_bundles["save"] = get_save_png_bundle();
    m_bitmap_bundles["save_dark"] = get_save_dark_png_bundle();
    m_bitmap_bundles["sim_add_plot"] = get_sim_add_plot_png_bundle();
    m_bitmap_bundles["sim_add_plot_dark"] = get_sim_add_plot_dark_png_bundle();
    m_bitmap_bundles["sim_command"] = get_sim_command_png_bundle();
    m_bitmap_bundles["sim_command_dark"] = get_sim_command_dark_png_bundle();
    m_bitmap_bundles["sim_run"] = get_sim_run_png_bundle();
    m_bitmap_bundles["sim_run_dark"] = get_sim_run_dark_png_bundle();
    m_bitmap_bundles["sim_tune"] = get_sim_tune_png_bundle();
    m_bitmap_bundles["sim_tune_dark"] = get_sim_tune_dark_png_bundle();
    m_bitmap_bundles["simulator"] = get_simulator_png_bundle();
    m_bitmap_bundles["window-icon"] = get_window_icon_png_bundle();
}
