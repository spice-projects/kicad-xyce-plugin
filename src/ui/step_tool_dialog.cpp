#include <sstream>
#include <vector>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#endif

#include "step_tool_dialog.h"

StepToolDialog::StepToolDialog(wxWindow* parent, const StepInformation* step_information, const std::set<size_t>& selected_steps) :
    wxDialog(parent, wxID_ANY, "Step Tool", wxDefaultPosition, wxSize(550, 400), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER), m_step_information(step_information), m_selected_steps(selected_steps) {
    // set minimum size
    SetMinSize(wxSize(550, 400));
    // create main vertical sizer
    auto main_sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(main_sizer);
    // create title label
    auto title = new wxStaticText(this, wxID_ANY, "Step Tool");
    wxFont title_font = title->GetFont();
    title_font.SetPointSize(16);
    title_font.SetWeight(wxFONTWEIGHT_BOLD);
    title->SetFont(title_font);
    main_sizer->Add(title, 0, wxTOP | wxLEFT | wxRIGHT, 14);
    // create subtitle label
    auto subtitle = new wxStaticText(this, wxID_ANY, "Select one or more parameter combinations to keep active for this chart");
    wxFont subtitle_font = subtitle->GetFont();
    subtitle_font.SetPointSize(10);
    subtitle->SetFont(subtitle_font);
    main_sizer->Add(subtitle, 0, wxTOP | wxLEFT | wxRIGHT, 4);
    // create action buttons row
    auto actions_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto select_all_btn = new wxButton(this, wxID_ANY, "Select All", wxDefaultPosition, wxSize(-1, 24));
    auto clear_all_btn = new wxButton(this, wxID_ANY, "Clear All", wxDefaultPosition, wxSize(-1, 24));
    auto invert_btn = new wxButton(this, wxID_ANY, "Invert", wxDefaultPosition, wxSize(-1, 24));
    actions_sizer->Add(select_all_btn, 0, wxRIGHT, 6);
    actions_sizer->Add(clear_all_btn, 0, wxRIGHT, 6);
    actions_sizer->Add(invert_btn, 0, wxRIGHT, 16);
    // selection count label
    m_selection_count_label = new wxStaticText(this, wxID_ANY, "");
    actions_sizer->Add(m_selection_count_label, 0, wxCENTER, 0);
    main_sizer->Add(actions_sizer, 0, wxALL, 12);
    // create list view for steps
    m_step_list = new wxListView(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxBORDER_THEME);
    m_step_list->EnableCheckBoxes(true);
    // add checkbox column
    m_step_list->AppendColumn("", wxLIST_FORMAT_LEFT, 60);
    // add parameter name columns
    for (const auto& column_name : m_step_information->keys())
        m_step_list->AppendColumn(column_name, wxLIST_FORMAT_RIGHT, 140);
    // step values
    const auto& all_values = m_step_information->values();
    // populate rows
    for (size_t step_index = 0; step_index < all_values.size(); ++step_index) {
        // insert item
        long item_index = m_step_list->InsertItem(m_step_list->GetItemCount(), wxString::Format("%zu", step_index));
        // store step index as item data
        m_step_list->SetItemData(item_index, step_index);
        // set checkbox state based on initial selection
        m_step_list->CheckItem(item_index, m_selected_steps.contains(step_index));
        // set parameter values as sub-items
        const auto& row_values = all_values[step_index];
        // loop through columns and set values
        for (size_t col = 0; col < row_values.size(); ++col) {
            // format value string
            std::ostringstream oss;
            oss << row_values[col];
            // set sub-item text for the column (col + 1 because column 0 is the checkbox)
            m_step_list->SetItem(item_index, static_cast<int>(col + 1), oss.str());
        }
    }
    // update selection count label
    update_selection_count();
    main_sizer->Add(m_step_list, 1, wxEXPAND | wxLEFT | wxRIGHT, 12);
    // create standard OK/Cancel button sizer using platform conventions
    main_sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 12);
    // bind action button events
    select_all_btn->Bind(wxEVT_BUTTON, &StepToolDialog::on_select_all, this);
    clear_all_btn->Bind(wxEVT_BUTTON, &StepToolDialog::on_clear_all, this);
    invert_btn->Bind(wxEVT_BUTTON, &StepToolDialog::on_invert_selection, this);
    // bind list item toggle events
    m_step_list->Bind(wxEVT_LIST_ITEM_CHECKED, &StepToolDialog::on_item_toggled, this);
    m_step_list->Bind(wxEVT_LIST_ITEM_UNCHECKED, &StepToolDialog::on_item_toggled, this);
}

std::set<size_t> StepToolDialog::selected_steps() const {
    // return a copy of the current selection set
    return m_selected_steps;
}

void StepToolDialog::on_select_all(wxCommandEvent&) {
    // clear current selection
    m_selected_steps.clear();
    // iterate all steps and select them
    for (size_t step_index = 0; step_index < m_step_information->length(); ++step_index) {
        // insert step index
        m_selected_steps.insert(step_index);
    }
    // update checkbox states
    for (int item = 0; item < m_step_list->GetItemCount(); ++item) {
        m_step_list->CheckItem(item, true);
    }
    // update label
    update_selection_count();
}

void StepToolDialog::on_clear_all(wxCommandEvent&) {
    // clear current selection
    m_selected_steps.clear();
    // update checkbox states
    for (int item = 0; item < m_step_list->GetItemCount(); ++item) {
        m_step_list->CheckItem(item, false);
    }
    // update label
    update_selection_count();
}

void StepToolDialog::on_invert_selection(wxCommandEvent&) {
    // iterate all steps
    for (size_t step_index = 0; step_index < m_step_information->length(); ++step_index) {
        // toggle selection
        if (m_selected_steps.contains(step_index)) {
            // remove from selected set
            m_selected_steps.erase(step_index);
        }
        else {
            // add to selected set
            m_selected_steps.insert(step_index);
        }
    }
    // update checkbox states
    for (int item = 0; item < m_step_list->GetItemCount(); ++item) {
        // get step index from item data
        size_t step_index = static_cast<size_t>(m_step_list->GetItemData(item));
        // update checkbox to match inverted state
        m_step_list->CheckItem(item, m_selected_steps.contains(step_index));
    }
    // update label
    update_selection_count();
}

void StepToolDialog::on_item_toggled(wxListEvent& event) {
    // get item index
    long item_index = event.GetIndex();
    // get step index from item data
    size_t step_index = static_cast<size_t>(m_step_list->GetItemData(item_index));
    // update selection set based on checkbox state
    if (m_step_list->IsItemChecked(item_index)) {
        // add to selected set
        m_selected_steps.insert(step_index);
    }
    else {
        // remove from selected set
        m_selected_steps.erase(step_index);
    }
    // update label
    update_selection_count();
}

void StepToolDialog::update_selection_count() {
    // update label text with current selection count
    m_selection_count_label->SetLabel(wxString::Format("Selected %zu / %zu", m_selected_steps.size(), m_step_information->length()));
}
