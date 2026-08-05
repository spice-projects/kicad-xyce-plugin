#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <algorithm>
#include <memory>

#include <d3d11.h>
#include <dxgi.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <implot.h>
#include <spdlog/spdlog.h>
#include <windows.h>
#include <wx/wx.h>

#include "charts_panel.h"
#include "wxwidgets_imgui.h"

static constexpr const char* FONT_PATH = KICAD_XYCE_FONTS_DIR "\\Inter-Regular.ttf";

struct ChartsPanelRenderContext
{
    // store the native window handle
    HWND hwnd = nullptr;
    // store the Direct3D 11 device
    ID3D11Device* device = nullptr;
    // store the Direct3D 11 device context
    ID3D11DeviceContext* device_context = nullptr;
    // store the swap chain
    IDXGISwapChain* swap_chain = nullptr;
    // store the render target view
    ID3D11RenderTargetView* render_target_view = nullptr;
    // store the current width
    UINT width = 0;
    // store the current height
    UINT height = 0;
    // store the initialization state
    bool initialized = false;
};

// store the render contexts by native window handle
static std::unordered_map<HWND, std::unique_ptr<ChartsPanelRenderContext>> s_render_contexts;

static ChartsPanelRenderContext& get_render_context(const HWND hwnd) {
    // lookup the existing render context
    auto it = s_render_contexts.find(hwnd);
    if (it == s_render_contexts.end()) {
        // create a new render context
        auto context = std::make_unique<ChartsPanelRenderContext>();
        // store the native window handle
        context->hwnd = hwnd;
        // insert the render context in the map
        auto [inserted_it, _] = s_render_contexts.emplace(hwnd, std::move(context));
        // store the iterator
        it = inserted_it;
    }
    // return the render context
    return *it->second;
}

static void release_render_context(ChartsPanelRenderContext& context) {
    // release the render target view if it exists
    if (context.render_target_view) {
        // release the object
        context.render_target_view->Release();
        // reset the pointer
        context.render_target_view = nullptr;
    }
    // release the swap chain if it exists
    if (context.swap_chain) {
        // release the object
        context.swap_chain->Release();
        // reset the pointer
        context.swap_chain = nullptr;
    }
    // release the device context if it exists
    if (context.device_context) {
        // release the object
        context.device_context->Release();
        // reset the pointer
        context.device_context = nullptr;
    }
    // release the device if it exists
    if (context.device) {
        // release the object
        context.device->Release();
        // reset the pointer
        context.device = nullptr;
    }
    // reset the dimensions
    context.width = 0;
    context.height = 0;
    // reset the initialization state
    context.initialized = false;
}

static bool create_render_target(ChartsPanelRenderContext& context) {
    // check the render context is valid
    if (!context.device || !context.swap_chain)
        return false;

    // release the old render target view if it exists
    if (context.render_target_view) {
        // release the old object
        context.render_target_view->Release();
        // reset the pointer
        context.render_target_view = nullptr;
    }

    // get the back buffer from the swap chain
    ID3D11Texture2D* back_buffer = nullptr;
    if (FAILED(context.swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer))))
        return false;

    // create the render target view
    const auto result = context.device->CreateRenderTargetView(back_buffer, nullptr, &context.render_target_view);
    // release the back buffer
    back_buffer->Release();
    // return the result
    return SUCCEEDED(result);
}

static bool resize_render_target(ChartsPanelRenderContext& context) {
    // check the render context is valid
    if (!context.swap_chain || !context.device || !context.hwnd)
        return false;

    // get the client rectangle
    RECT client_rect{};
    if (!::GetClientRect(context.hwnd, &client_rect))
        return false;

    // compute the current width and height
    const auto width = (std::max)(LONG{1}, client_rect.right - client_rect.left);
    const auto height = (std::max)(LONG{1}, client_rect.bottom - client_rect.top);

    // check the size did not change
    if (context.width == static_cast<UINT>(width) && context.height == static_cast<UINT>(height))
        return true;

    // release the old render target view if it exists
    if (context.render_target_view) {
        // release the old object
        context.render_target_view->Release();
        // reset the pointer
        context.render_target_view = nullptr;
    }

    // resize the swap chain
    const auto result = context.swap_chain->ResizeBuffers(2, static_cast<UINT>(width), static_cast<UINT>(height), DXGI_FORMAT_B8G8R8A8_UNORM, 0);
    if (FAILED(result)) {
        // log the error
        spdlog::error("Failed to resize Direct3D 11 swap chain for charts panel");
        // exit
        return false;
    }

    // recreate the render target view
    if (!create_render_target(context)) {
        // log the error
        spdlog::error("Failed to recreate the Direct3D 11 render target view for charts panel");
        // exit
        return false;
    }

    // update the dimensions
    context.width = static_cast<UINT>(width);
    context.height = static_cast<UINT>(height);

    // create the viewport
    D3D11_VIEWPORT viewport{};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<float>(context.width);
    viewport.Height = static_cast<float>(context.height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    // bind the viewport to the device context
    context.device_context->RSSetViewports(1, &viewport);
    // exit
    return true;
}

void ChartsPanel::initialize() {
    // get the native window handle
    m_charts_panel = GetHandle();
    const auto hwnd = reinterpret_cast<HWND>(m_charts_panel);
    if (!hwnd)
        return;

    // get the render context for this window
    auto& context = get_render_context(hwnd);
    if (context.initialized)
        return;

    // get the client rectangle
    RECT client_rect{};
    if (!::GetClientRect(hwnd, &client_rect)) {
        // log the error
        spdlog::error("Failed to retrieve the client area for the charts panel");
        // exit
        return;
    }

    // compute the panel size
    const auto width = (std::max)(LONG{1}, client_rect.right - client_rect.left);
    const auto height = (std::max)(LONG{1}, client_rect.bottom - client_rect.top);

    // create the Direct3D 11 device
    D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;
    UINT create_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
    // enable debug layer in debug builds
    create_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    // create the device and context
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* device_context = nullptr;
    const D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_0};
    if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, create_flags, feature_levels, 1, D3D11_SDK_VERSION, &device, &feature_level, &device_context))) {
        // log the error
        spdlog::error("Failed to create the Direct3D 11 device for the charts panel");
        // exit
        return;
    }

    // create the DXGI factory
    IDXGIFactory* factory = nullptr;
    if (FAILED(CreateDXGIFactory(IID_PPV_ARGS(&factory)))) {
        // release the device context
        device_context->Release();
        // release the device
        device->Release();
        // log the error
        spdlog::error("Failed to create the DXGI factory for the charts panel");
        // exit
        return;
    }

    // configure the swap chain
    DXGI_SWAP_CHAIN_DESC swap_chain_desc{};
    swap_chain_desc.BufferDesc.Width = static_cast<UINT>(width);
    swap_chain_desc.BufferDesc.Height = static_cast<UINT>(height);
    swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
    swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
    swap_chain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swap_chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swap_chain_desc.SampleDesc.Count = 1;
    swap_chain_desc.SampleDesc.Quality = 0;
    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_desc.BufferCount = 2;
    swap_chain_desc.OutputWindow = hwnd;
    swap_chain_desc.Windowed = TRUE;
    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    // create the swap chain
    IDXGISwapChain* swap_chain = nullptr;
    if (FAILED(factory->CreateSwapChain(device, &swap_chain_desc, &swap_chain))) {
        // release the factory
        factory->Release();
        // release the device context
        device_context->Release();
        // release the device
        device->Release();
        // log the error
        spdlog::error("Failed to create the Direct3D 11 swap chain for the charts panel");
        // exit
        return;
    }

    // release the factory
    factory->Release();

    // store the device and swap chain in the render context
    context.hwnd = hwnd;
    context.device = device;
    context.device_context = device_context;
    context.swap_chain = swap_chain;
    context.width = static_cast<UINT>(width);
    context.height = static_cast<UINT>(height);

    // create the render target view
    if (!create_render_target(context)) {
        // release the render context
        release_render_context(context);
        // log the error
        spdlog::error("Failed to create the Direct3D 11 render target view for the charts panel");
        // exit
        return;
    }

    // create isolated contexts for this panel
    initialize_contexts();
    // activate this panel's isolated contexts for backend initialization
    ContextScope context_scope(*this);
    // style
    PlatformStyle();
    // DPI scaling for high-DPI displays
    HDC hdc = GetDC(hwnd);
    const float scale = GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;
    ReleaseDC(hwnd, hdc);

    // ImGui configuration
    ImGuiIO& io = ImGui::GetIO();
    // font base size
    const float base_size = 14.0f;
    // add font with DPI-aware scaling
    io.Fonts->AddFontFromFileTTF(FONT_PATH, base_size * scale);
    io.FontGlobalScale = 1.0f / scale;
    // update style
    ImGuiStyle& style = ImGui::GetStyle();
    style.AntiAliasedLines = true;
    style.AntiAliasedLinesUseTex = true;

    // initialize the Direct3D 11 backend
    if (!ImGui_ImplDX11_Init(device, device_context)) {
        // log the error
        spdlog::error("Failed to initialize the Dear ImGui Direct3D 11 backend for the charts panel");
        // release the render context
        release_render_context(context);
        // release the isolated contexts
        terminate_contexts();
        // exit
        return;
    }

    // mark the render context as initialized
    context.initialized = true;
}

void ChartsPanel::terminate() {
    // check the native window handle
    const auto hwnd = reinterpret_cast<HWND>(m_charts_panel);
    if (!hwnd)
        return;

    // lookup the render context
    auto it = s_render_contexts.find(hwnd);
    if (it != s_render_contexts.end()) {
        // get the render context
        auto& context = *it->second;
        if (context.initialized) {
            // preserve the active contexts while releasing this panel's renderer
            auto* previous_imgui_context = ImGui::GetCurrentContext();
            auto* previous_implot_context = ImPlot::GetCurrentContext();
            // activate this panel's isolated contexts for backend teardown
            ImGui::SetCurrentContext(static_cast<ImGuiContext*>(m_imgui_context));
            ImPlot::SetCurrentContext(static_cast<ImPlotContext*>(m_implot_context));
            // shutdown the Direct3D backend
            ImGui_ImplDX11_Shutdown();
            // restore the contexts active before renderer teardown
            ImPlot::SetCurrentContext(previous_implot_context);
            ImGui::SetCurrentContext(previous_imgui_context);
        }
        // release isolated contexts after the renderer backend
        terminate_contexts();
        // release the render context resources
        release_render_context(context);
        // erase the render context from the map
        s_render_contexts.erase(it);
    }

    // reset the panel handle
    m_charts_panel = nullptr;
}

void ChartsPanel::render_frame(const std::function<void()>& renderer) {
    // check the native window handle
    const auto hwnd = reinterpret_cast<HWND>(m_charts_panel);
    if (!hwnd)
        return;

    // get the render context
    auto& context = get_render_context(hwnd);
    if (!context.initialized || !context.device_context || !context.swap_chain || !context.render_target_view)
        return;

    // activate this panel's isolated contexts for the complete frame
    ContextScope context_scope(*this);
    // resize the render target if needed
    resize_render_target(context);

    // get the render target view
    ID3D11RenderTargetView* render_target_view = context.render_target_view;
    // bind the render target
    context.device_context->OMSetRenderTargets(1, &render_target_view, nullptr);
    // clear the render target with the panel background color
    const float clear_color[4] = {m_background_color.x, m_background_color.y, m_background_color.z, m_background_color.w};
    context.device_context->ClearRenderTargetView(context.render_target_view, clear_color);

    // start a new ImGui frame
    ImGui_ImplDX11_NewFrame();
    // update the per-panel frame duration
    update_delta_time();
    ImGui::NewFrame();

    // get the panel client size
    const wxSize client_size = GetClientSize();
    // DPI scaling for high-DPI displays
    HDC hdc = GetDC(hwnd);
    const float dpi_scale = GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;
    ReleaseDC(hwnd, hdc);
    // configure the ImGui IO size
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(static_cast<float>(client_size.x), static_cast<float>(client_size.y));
    io.DisplayFramebufferScale = ImVec2(dpi_scale, dpi_scale);

    // set the next window position and size
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(client_size.x), static_cast<float>(client_size.y)));

    // render the chart contents
    renderer();

    // render the ImGui frame
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    // present the swap chain without waiting for v-sync
    context.swap_chain->Present(0, 0);
}

bool ChartsPanel::update_bounds() {
    // check the native window handle
    const auto hwnd = reinterpret_cast<HWND>(m_charts_panel);
    if (!hwnd)
        return false;
    // lookup the render context
    auto it = s_render_contexts.find(hwnd);
    if (it == s_render_contexts.end())
        return false;
    // resize the render target if needed
    resize_render_target(*it->second);
    // exit
    return true;
}

void ChartsPanel::display_changed() {
    // recompute the decimation target for the new display scale
    update_decimation_target();
    // update the bounds
    update_bounds();
}
