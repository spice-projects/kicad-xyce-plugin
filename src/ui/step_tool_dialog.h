#pragma once

#include <set>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/dialog.h>
#include <wx/listctrl.h>
#include <wx/stattext.h>
#endif

#include "../step_information.h"

class StepToolDialog : public wxDialog
{
public:
    StepToolDialog(wxWindow* parent, const StepInformation* step_information, const std::set<size_t>& selected_steps);

    [[nodiscard]] std::set<size_t> selected_steps() const;

private:
    const StepInformation* m_step_information;
    wxListView* m_step_list = nullptr;
    std::set<size_t> m_selected_steps;
    wxStaticText* m_selection_count_label = nullptr;

    void on_select_all(wxCommandEvent& event);

    void on_clear_all(wxCommandEvent& event);

    void on_invert_selection(wxCommandEvent& event);

    void on_item_toggled(wxListEvent& event);

    void update_selection_count();
};
