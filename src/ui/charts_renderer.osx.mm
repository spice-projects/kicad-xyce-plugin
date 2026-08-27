#import <Metal/Metal.h>

#include <gpu/ganesh/GrDirectContext.h>
#include <gpu/ganesh/mtl/GrMtlBackendContext.h>
#include <gpu/ganesh/mtl/GrMtlDirectContext.h>
#include <ports/SkCFObject.h>

#include "charts_renderer.h"

sk_sp<GrDirectContext> ChartsRenderer::create_gpu_context() {
    // create metal device
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (!device)
        return nullptr;
    // create command queue
    id<MTLCommandQueue> queue = [device newCommandQueue];
    if (!queue) {
        // release device if queue creation fails
        [device release];
        // exit
        return nullptr;
    }
    // create backend context
    GrMtlBackendContext backend;
    // use device and queue to create backend context
    backend.fDevice = sk_cfp<GrMTLHandle>(device);
    backend.fQueue = sk_cfp<GrMTLHandle>(queue);
    // create direct context
    return GrDirectContexts::MakeMetal(backend);
}
