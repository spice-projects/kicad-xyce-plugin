#pragma once

#include <mutex>
#include <unordered_map>

struct MetalGPUHandle
{
    void* device = nullptr;
    void* command_queue = nullptr;
};

namespace MTL
{
    class Device;
    class CommandQueue;
} // namespace MTL

class MetalResourceManager
{
public:
    static MetalResourceManager* get_instance();

    MetalGPUHandle get_gpu(void* handle);

private:
    std::mutex m_mutex;
    std::unordered_map<void*, MetalGPUHandle> m_cache;
    void* m_default_device = nullptr;
    void* m_default_queue = nullptr;
};
