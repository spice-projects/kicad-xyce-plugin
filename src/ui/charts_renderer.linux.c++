#include "charts_renderer.h"

sk_sp<GrDirectContext> ChartsRenderer::create_gpu_context() {
    // use cpu rasterizer for now, since we don't have a gpu context on linux yet
    return nullptr;
}
