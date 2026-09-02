#define WIN32_LEAN_AND_MEAN

#include <d3d12.h>
#include <dxgi1_4.h>

#include <gpu/ganesh/GrDirectContext.h>
#include <gpu/ganesh/d3d/GrD3DBackendContext.h>
#include <gpu/ganesh/d3d/GrD3DDirectContext.h>

#include "charts_renderer.h"

sk_sp<GrDirectContext> ChartsRenderer::create_gpu_context() {
    // create dxgi factory for adapter enumeration
    IDXGIFactory1* factory = nullptr;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))))
        return nullptr;
    // enumerate adapters to find the first hardware adapter
    IDXGIAdapter1* adapter = nullptr;
    for (UINT adapter_index = 0; factory->EnumAdapters1(adapter_index, &adapter) != DXGI_ERROR_NOT_FOUND; ++adapter_index) {
        // describe the adapter
        DXGI_ADAPTER_DESC1 adapter_desc;
        adapter->GetDesc1(&adapter_desc);
        // skip software adapters
        if (adapter_desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            // release the software adapter
            adapter->Release();
            // reset for the next iteration
            adapter = nullptr;
            // try the next adapter
            continue;
        }
        // use the first hardware adapter
        break;
    }
    // release the factory, it is no longer needed after adapter enumeration
    factory->Release();
    // check no suitable adapter was found
    if (adapter == nullptr)
        return nullptr;
    // create d3d12 device
    ID3D12Device* device = nullptr;
    if (FAILED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)))) {
        // release adapter if device creation fails
        adapter->Release();
        // exit
        return nullptr;
    }
    // create command queue
    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    ID3D12CommandQueue* queue = nullptr;
    if (FAILED(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&queue)))) {
        // release device if queue creation fails
        device->Release();
        // release adapter if queue creation fails
        adapter->Release();
        // exit
        return nullptr;
    }
    // create backend context
    GrD3DBackendContext backend;
    // use adapter, device and queue to create backend context, gr_cp AddRefs the raw pointers, release the originals to prevent leaks
    backend.fAdapter = gr_cp<IDXGIAdapter1>(adapter);
    backend.fDevice = gr_cp<ID3D12Device>(device);
    backend.fQueue = gr_cp<ID3D12CommandQueue>(queue);
    // create direct context, before releasing the original references to prevent premature destruction of the objects, gr_cp has AddRef'd them
    return GrDirectContexts::MakeD3D(backend);
}
