#pragma once

#include <functional>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include <wx/arrstr.h>
#include <wx/string.h>

class wxChoice;
class wxTextCtrl;

namespace wx_util
{
    // convert a range of string-like values (std::vector<std::string>,
    // std::array<...>, ...) into the wxArrayString expected by wxChoice
    template <typename Range>
    [[nodiscard]] wxArrayString to_wx_array_string(const Range& values) {
        wxArrayString result;
        // add each value, converting to wxString on the fly
        for (const auto& value : values) {
            result.Add(wxString(value));
        }
        return result;
    }

    // convert brace-enclosed string literals into a wxArrayString
    [[nodiscard]] inline wxArrayString to_wx_array_string(std::initializer_list<const char*> values) {
        wxArrayString result;
        // add each literal, converting to wxString on the fly
        for (const char* value : values) {
            result.Add(wxString(value));
        }
        return result;
    }

    // convert a wxString to a UTF-8 std::string
    [[nodiscard]] inline std::string to_std_string(const wxString& value) { return std::string(value.ToUTF8()); }

    // convert a UTF-8 std::string to a wxString
    [[nodiscard]] inline wxString to_wx_string(std::string_view value) { return wxString::FromUTF8(value.data(), value.size()); }

    // split multi-line text into non-empty trimmed lines
    [[nodiscard]] std::vector<std::string> split_lines(const wxString& text);

    // split text into trimmed non-empty tokens separated by any delimiter character
    [[nodiscard]] std::vector<std::string> split_strings(const wxString& text, std::string_view delimiters = " \t\r\n");

    // join string-like values into a single wxString using the given separator
    template <typename Range>
    [[nodiscard]] wxString join_strings(const Range& values, std::string_view separator);

    // join values serialized by a transform into a single wxString using the given separator
    template <typename Range, typename Transform>
    [[nodiscard]] wxString join_strings(const Range& values, std::string_view separator, Transform&& transform);

    // read the current value of a text control as a UTF-8 std::string
    [[nodiscard]] std::string get_text(const wxTextCtrl& text_ctrl);

    // restore a text control value from a UTF-8 std::string
    void set_text(wxTextCtrl& text_ctrl, std::string_view value);

    // read the current selection of a choice control as a UTF-8 std::string
    [[nodiscard]] std::string get_string_selection(const wxChoice& choice);

    // select the choice entry matching value, falling back to the first entry
    void set_choice_by_string(wxChoice& choice, std::string_view value);

    template <typename Range>
    [[nodiscard]] wxString join_strings(const Range& values, std::string_view separator) {
        wxString result;
        // track whether the separator is still pending
        bool first = true;
        // append each value, inserting the separator before later entries
        for (const auto& value : values) {
            if (first) {
                first = false;
            }
            else {
                result += to_wx_string(separator);
            }
            result += wxString(value);
        }
        return result;
    }

    template <typename Range, typename Transform>
    [[nodiscard]] wxString join_strings(const Range& values, std::string_view separator, Transform&& transform) {
        wxString result;
        // track whether the separator is still pending
        bool first = true;
        // append each serialized value, inserting the separator before later entries
        for (const auto& value : values) {
            if (first) {
                first = false;
            }
            else {
                result += to_wx_string(separator);
            }
            result += to_wx_string(std::invoke(transform, value));
        }
        return result;
    }
} // namespace wx_util
