#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace ipc {

// Cross-platform shared-memory framebuffer for zero-copy frame transport
// between the Runtime (writer) and Manager (reader) processes.
//
// Runtime side:  Create() → write pixels → notify via IPC
// Manager side:  Open()   → read pixels on IPC notification
class SharedFramebuffer {
public:
    SharedFramebuffer() = default;
    ~SharedFramebuffer();

    SharedFramebuffer(const SharedFramebuffer&) = delete;
    SharedFramebuffer& operator=(const SharedFramebuffer&) = delete;

    // Runtime side: create a new shared memory region.
    // Returns false on failure. Destroys any previous mapping first.
    bool Create(const std::string& name, uint32_t width, uint32_t height);

    // Manager side: open an existing shared memory region created by the runtime.
    bool Open(const std::string& name, uint32_t width, uint32_t height);

    // Close and unmap. If this side created the shm, also unlinks/destroys it.
    void Close();

    bool IsValid() const { return data_ != nullptr; }

    uint8_t* data() const { return data_; }
    size_t size() const { return size_; }
    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }
    uint32_t stride() const { return width_ * 4; }
    const std::string& name() const { return name_; }

private:
    uint8_t* data_ = nullptr;
    size_t size_ = 0;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    std::string name_;
    bool is_owner_ = false;

#ifdef _WIN32
    void* map_handle_ = nullptr;
#else
    int shm_fd_ = -1;
#endif
};

// Generate a shared framebuffer name for a given VM ID.
std::string GetSharedFramebufferName(const std::string& vm_id);

}  // namespace ipc
