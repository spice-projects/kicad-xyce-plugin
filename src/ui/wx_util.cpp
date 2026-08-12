#include <sstream>
#include <string>
#include <vector>

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/arrstr.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/tokenzr.h>
#endif

#include "wx_util.h"

namespace wx_util
{
    std::vector<std::string> split_lines(const wxString& text) {
        // output lines
        std::vector<std::string> lines;
        std::istringstream stream(std::string(text.ToUTF8()));
        std::string line;
        // process each line of the input
        while (std::getline(stream, line)) {
            // trim leading and trailing whitespace
            size_t start = line.find_first_not_of(" \t\r");
            if (start == std::string::npos)
                continue;
            size_t end = line.find_last_not_of(" \t\r");
            line = line.substr(start, end - start + 1);
            // skip empty lines
            if (!line.empty())
                lines.push_back(std::move(line));
        }
        return lines;
    }

    std::vector<std::string> split_strings(const wxString& text, std::string_view delimiters) {
        // output tokens
        std::vector<std::string> tokens;
        wxStringTokenizer tokenizer(text, wxString::FromUTF8(delimiters.data(), delimiters.size()));
        while (tokenizer.HasMoreTokens()) {
            // trim the token of surrounding whitespace
            wxString token = tokenizer.GetNextToken();
            token.Trim(true).Trim(false);
            // skip empty tokens
            if (!token.IsEmpty()) {
                tokens.push_back(std::string(token.ToUTF8()));
            }
        }
        return tokens;
    }

    std::string get_text(const wxTextCtrl& text_ctrl) { return to_std_string(text_ctrl.GetValue()); }

    void set_text(wxTextCtrl& text_ctrl, std::string_view value) { text_ctrl.SetValue(to_wx_string(value)); }

    std::string get_string_selection(const wxChoice& choice) { return to_std_string(choice.GetStringSelection()); }

    void set_choice_by_string(wxChoice& choice, std::string_view value) {
        // select the matching entry or fall back to the first entry
        int index = choice.FindString(to_wx_string(value));
        choice.SetSelection(index != wxNOT_FOUND ? index : 0);
    }
} // namespace wx_util
