#pragma once
#include <eldr/vulkan/commandbuffer.hpp>
#include <eldr/vulkan/common.hpp>
#include <eldr/vulkan/device.hpp>
#include <eldr/vulkan/vertex.hpp>

namespace eldr {
namespace vk {

struct BufferInfo {
  VkDeviceSize          size;
  VkBufferUsageFlags    usage;
  VkMemoryPropertyFlags properties;
};

class Buffer {
public:
  Buffer();
  Buffer(const Device*, const BufferInfo&);
  Buffer(const Device*, const std::vector<Vertex>&,
         CommandPool&); // Vertex buffer
  Buffer(const Device*, const std::vector<uint32_t>&,
         CommandPool&); // Index buffer
  Buffer(Buffer&&) = default;
  ~Buffer();

  Buffer& operator=(Buffer&&);

  VkDeviceSize          size() const { return size_; }
  const VkBuffer&       get() const { return buffer_; }
  const VkDeviceMemory& memory() const { return buffer_memory_; }
  void                  copyFrom(Buffer&, CommandPool&);

protected:
  const Device* device_;

  VkBuffer       buffer_;
  VkDeviceMemory buffer_memory_;
  VkDeviceSize   size_;
};
} // namespace vk
} // namespace eldr
