#pragma once

#include <string>
#include <unordered_map>

#include <wx/wx.h>

class IconManager
{
public:
    static IconManager& get() {
        // declare and create a static instance of IconManager
        static IconManager instance;
        // use instance for all subsequent calls
        return instance;
    }

    wxBitmapBundle get_bitmap(bool dark, const std::string& key) const {
        // look up the bitmap bundle in the map
        auto it = m_bitmap_bundles.find(key + (dark ? "_dark" : ""));
        // if found and valid, return the bitmap bundle
        if (it != m_bitmap_bundles.end() && it->second.IsOk()) {
            // return the bitmap bundle
            return it->second;
        }
        return wxBitmapBundle();
    }

private:
    std::unordered_map<std::string, wxBitmapBundle> m_bitmap_bundles;

    IconManager();
};
