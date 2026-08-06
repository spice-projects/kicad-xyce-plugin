#pragma once

#include "charts_panel.h"

class ContextScope
{
public:
    explicit ContextScope(const ChartsPanel& charts_panel);

    ~ContextScope();

    ContextScope(const ContextScope&) = delete;

    ContextScope& operator=(const ContextScope&) = delete;

private:
    void* m_imgui_context = nullptr;
    void* m_implot_context = nullptr;
};
