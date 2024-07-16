#include <eldr/core/logger.hpp>
#include <eldr/vulkan/buffer.hpp>
#include <eldr/vulkan/helpers.hpp>

namespace eldr {
namespace vk {

Buffer::Buffer()
  : device_(nullptr), buffer_(VK_NULL_HANDLE), buffer_memory_(VK_NULL_HANDLE)
{
}

Buffer::Buffer(const Device* device, const BufferInfo& buffer_info)
  : device_(device), size_(buffer_info.size)
{
  if (buffer_info.size == 0) {
    buffer_        = VK_NULL_HANDLE;
    buffer_memory_ = VK_NULL_HANDLE;
    return;
  }

  VkBufferCreateInfo buffer_ci{};
  buffer_ci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_ci.size        = size_;
  buffer_ci.usage       = buffer_info.usage;
  buffer_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device_->logical(), &buffer_ci, nullptr, &buffer_) !=
      VK_SUCCESS)
    ThrowVk("Failed to create buffer!");

  VkMemoryRequirements mem_requirements;
  vkGetBufferMemoryRequirements(device_->logical(), buffer_, &mem_requirements);

  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex =
    findMemoryType(device_->physical(), mem_requirements.memoryTypeBits,
                   buffer_info.properties);

  if (vkAllocateMemory(device_->logical(), &alloc_info, nullptr,
                       &buffer_memory_) != VK_SUCCESS)
    ThrowVk("Failed to allocate buffer memory!");

  vkBindBufferMemory(device_->logical(), buffer_, buffer_memory_, 0);
}

Buffer::Buffer(const Device* device, const std::vector<Vertex>& vertices,
               CommandPool& command_pool)
  : Buffer(device, { sizeof(vertices[0]) * vertices.size(),
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT })
{
  if (size_ == 0)
    return;

  Buffer staging_buffer(device, { size_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT });

  void* data;
  vkMapMemory(device->logical(), staging_buffer.memory(), 0, size_, 0, &data);
  memcpy(data, vertices.data(), (size_t) size_);
  vkUnmapMemory(device_->logical(), staging_buffer.memory());

  copyFrom(staging_buffer, command_pool);
}

Buffer::Buffer(const Device* device, const std::vector<uint32_t>& indices,
               CommandPool& command_pool)
  : Buffer(device, { sizeof(indices[0]) * indices.size(),
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                       VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT })
{
  if (size_ == 0)
    return;

  Buffer staging_buffer(device, { size_, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT });

  void* data;
  vkMapMemory(device->logical(), staging_buffer.memory(), 0, size_, 0, &data);
  memcpy(data, indices.data(), (size_t) size_);
  vkUnmapMemory(device_->logical(), staging_buffer.memory());

  copyFrom(staging_buffer, command_pool);
}

Buffer::~Buffer()
{
  if (buffer_ != VK_NULL_HANDLE)
    vkDestroyBuffer(device_->logical(), buffer_, nullptr);
  if (buffer_memory_ != VK_NULL_HANDLE)
    vkFreeMemory(device_->logical(), buffer_memory_, nullptr);
}

Buffer& Buffer::operator=(Buffer&& other)
{
  if (this != &other) {
    if (buffer_ != VK_NULL_HANDLE)
      vkDestroyBuffer(device_->logical(), buffer_, nullptr);
    if (buffer_memory_ != VK_NULL_HANDLE)
      vkFreeMemory(device_->logical(), buffer_memory_, nullptr);

    device_        = other.device_;
    buffer_        = other.buffer_;
    buffer_memory_ = other.buffer_memory_;
    size_          = other.size_;

    other.device_        = nullptr;
    other.buffer_        = VK_NULL_HANDLE;
    other.buffer_memory_ = VK_NULL_HANDLE;
    other.size_          = 0;
  }

  return *this;
}

// TODO: I don't like that command pool and graphics queue is passed
// here through the buffer constructor. Should organize this differently
void Buffer::copyFrom(Buffer& other, CommandPool& command_pool)
{
  CommandBuffer cb(device_, &command_pool);
  cb.begin();
  VkBufferCopy copy_region{}; // There are optional offsets in this struct
  copy_region.size = size_;
  vkCmdCopyBuffer(cb.get(), other.buffer_, buffer_, 1, &copy_region);
  cb.submit();
}

} // namespace vk
} // namespace eldr
