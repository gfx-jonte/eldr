#include <eldr/core/fstream.hpp>
#include <eldr/core/logger.hpp>
#include <eldr/render/scene.hpp>
#include <eldr/vulkan/buffer.hpp>
#include <eldr/vulkan/helpers.hpp>
#include <eldr/vulkan/texture.hpp>

namespace eldr {
namespace vk {

Texture::Texture(const Device* device, CommandPool& command_pool)
{
  const char* env_p = std::getenv("ELDR_DIR");
  if (env_p == nullptr) {
    Throw("Environment not set up correctly");
  }
  // FIXME: set this some other way
  const std::string     texture_path = "/textures/viking_room.png";
  std::filesystem::path filepath     = std::string(env_p) + texture_path;

  // use bitmap class somehow
  auto bitmap = std::make_unique<Bitmap>(filepath);
  if (bitmap->pixelFormat() != Bitmap::PixelFormat::RGBA)
    bitmap->rgbToRgba();

  mip_levels_ = static_cast<uint32_t>(std::floor(
                  std::log2(std::max(bitmap->width(), bitmap->height())))) +
                1;

  VkDeviceSize image_size =
    bitmap->width() * bitmap->height() * bitmap->channelCount();

  BufferInfo buffer_info{ .size       = image_size,
                          .usage      = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

  Buffer staging_buffer(device, buffer_info);

  void* data;
  vkMapMemory(device->logical(), staging_buffer.memory(), 0, image_size, 0,
              &data);
  memcpy(data, bitmap->data(), static_cast<size_t>(image_size));
  vkUnmapMemory(device->logical(), staging_buffer.memory());

  ImageInfo image_info{ .extent = { bitmap->width(), bitmap->height() },
                        .format = VK_FORMAT_R8G8B8A8_SRGB,
                        .tiling = VK_IMAGE_TILING_OPTIMAL,
                        .usage  = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                 VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                 VK_IMAGE_USAGE_SAMPLED_BIT,
                        .properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        .num_samples = VK_SAMPLE_COUNT_1_BIT,
                        .mip_levels = mip_levels_ };

  image_ = Image(device, image_info);

  image_.transitionLayout(command_pool, VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mip_levels_);

  image_.copyFromBuffer(staging_buffer, command_pool);

  //  image_.transitionLayout(command_pool,
  //  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
  //                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  //                          mip_levels_);

  // TODO: real-time generation of mipmaps like this is not common practice.
  // Add support for loading each mip level from disk
  generateMipmaps(device, command_pool, image_, mip_levels_);

  image_view_ = ImageView(device, image_.get(), image_.format(), mip_levels_);
}

void generateMipmaps(const Device* device, CommandPool& command_pool,
                     const Image& image, uint32_t mip_levels)
{
  VkFormatProperties format_props{};
  vkGetPhysicalDeviceFormatProperties(device->physical(), image.format(),
                                      &format_props);
  // Support for linear blitting is currently required
  if (!(format_props.optimalTilingFeatures &
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
    Throw("generateMipmaps(): texture image format does not support linear "
          "blitting!");
  }

  CommandBuffer cb(device, &command_pool);
  cb.beginSingleCommand();

  VkImageMemoryBarrier barrier{};
  barrier.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.image                       = image.get();
  barrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount     = 1;
  barrier.subresourceRange.levelCount     = 1;

  int32_t mip_width  = image.size().x;
  int32_t mip_height = image.size().y;

  for (uint32_t i = 1; i < mip_levels; ++i) {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    vkCmdPipelineBarrier(cb.get(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &barrier);

    VkImageBlit blit{};
    blit.srcOffsets[0]                 = { 0, 0, 0 };
    blit.srcOffsets[1]                 = { mip_width, mip_height, 1 };
    blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel       = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount     = 1;
    blit.dstOffsets[0]                 = { 0, 0, 0 };
    blit.dstOffsets[1]                 = { mip_width > 1 ? mip_width / 2 : 1,
                           mip_height > 1 ? mip_height / 2 : 1, 1 };
    blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel       = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount     = 1;
    vkCmdBlitImage(cb.get(), image.get(),
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image.get(),
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                   VK_FILTER_LINEAR);

    barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cb.get(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                         0, nullptr, 1, &barrier);

    if (mip_width > 1)
      mip_width /= 2;
    if (mip_height > 1)
      mip_height /= 2;
  }

  barrier.subresourceRange.baseMipLevel = mip_levels - 1;
  barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(cb.get(), VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);
  cb.submit();
}

// Texture& Texture::operator=(Texture&& other) {
//   if (this != &other) {
//     image_ = other.image_;
//     image_view_ = other.image_view_;
//   }
//   return *this;
// }

} // namespace vk
} // namespace eldr
