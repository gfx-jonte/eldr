#include <eldr/core/fwd.hpp>
#include <eldr/core/logger.hpp>
#include <eldr/core/math.hpp>
#include <eldr/vulkan/common.hpp>
#include <eldr/vulkan/vertex.hpp>
#include <eldr/vulkan/vulkan.hpp>

#include <string>

namespace eldr {
namespace vk {
struct UniformBufferObject {
  ELDR_IMPORT_CORE_TYPES();
  alignas(16) Mat4f model;
  alignas(16) Mat4f view;
  alignas(16) Mat4f proj;
};

const std::vector<Vertex> vertices = {
  { { -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
  { { 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
  { { 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
  { { -0.5f, 0.5f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } }
};

const std::vector<uint16_t> indices = { 0, 1, 2, 2, 3, 0 };

// fwd
// -----------------------------------------------------------------------------
static VkDebugUtilsMessengerEXT
setupDebugMessenger(VkInstance& instance, VkAllocationCallbacks* allocator);
// -----------------------------------------------------------------------------

VulkanWrapper::VulkanWrapper(GLFWwindow* const         window,
                             std::vector<const char*>& instance_extensions)
  : window_(window), current_frame_(0), instance_(instance_extensions),
    debug_messenger_(setupDebugMessenger(instance_.get(), nullptr)),
    surface_(&instance_, window), device_(instance_, surface_),
    swapchain_(&device_, surface_, window),
    render_pass_(&device_, swapchain_.format()),
    descriptor_set_layout_(&device_), descriptor_pool_(&device_),
    pipeline_(&device_, swapchain_, render_pass_, descriptor_set_layout_),
    command_pool_(&device_, surface_),
    texture_sampler_(&device_, command_pool_),
    vertex_buffer_(&device_, vertices, command_pool_),
    index_buffer_(&device_, indices, command_pool_),
    uniform_buffers_(max_frames_in_flight),
    uniform_buffers_mapped_(max_frames_in_flight),
    command_buffers_(max_frames_in_flight),
    descriptor_sets_(max_frames_in_flight)
{
  swapchain_.createFramebuffers(render_pass_);

  createUniformBuffers();

  createCommandBuffers();

  createDescriptorSets();

  // Sync objects
  for (uint8_t i = 0; i < max_frames_in_flight; ++i) {
    image_available_sem_.emplace_back(Semaphore(&device_));
    render_finished_sem_.emplace_back(Semaphore(&device_));
    in_flight_fences_.emplace_back(Fence(&device_));
  }
}

VulkanWrapper::~VulkanWrapper()
{
  vkDeviceWaitIdle(device_.logical());
#ifdef ELDR_VULKAN_DEBUG_REPORT
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
    instance_.get(), "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr)
    func(instance_.get(), debug_messenger_, nullptr);
#endif
}

#ifdef ELDR_VULKAN_DEBUG_REPORT
static VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugReportCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT             messageType,
  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
  (void) pUserData; // TODO: figure out how to use
  std::string type{};
  switch (messageType) {
    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
      type = "general";
      break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
      type = "validation";
      break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
      type = "performance";
      break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_FLAG_BITS_MAX_ENUM_EXT:
      type = "flag bits max"; // what even
      break;
    default:
      type = "invalid";
      break;
  }

  switch (messageSeverity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      spdlog::debug("[VULKAN DBG ({})]: {}", pCallbackData->pMessage, type);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      spdlog::info("[VULKAN DBG ({})]: {}", pCallbackData->pMessage, type);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      spdlog::warn("[VULKAN DBG ({})]: {}", pCallbackData->pMessage, type);
      break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      spdlog::error("[VULKAN DBG ({})]: {}", pCallbackData->pMessage, type);
      break;
    default:
      break;
  }
  return VK_FALSE;
}

VkDebugUtilsMessengerEXT setupDebugMessenger(VkInstance&            instance,
                                             VkAllocationCallbacks* allocator)
{
  VkDebugUtilsMessengerEXT debug_messenger;
  // Debug report callback
  VkDebugUtilsMessengerCreateInfoEXT debug_report_ci{};

  debug_report_ci.sType =
    VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  debug_report_ci.messageSeverity =
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  debug_report_ci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  debug_report_ci.pfnUserCallback = vkDebugReportCallback;
  debug_report_ci.pUserData       = nullptr; // Optional

  auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
    instance, "vkCreateDebugUtilsMessengerEXT");
  assert(func != nullptr);
  VkResult err = func(instance, &debug_report_ci, allocator, &debug_messenger);
  if (err != VK_SUCCESS)
    ThrowVk("Failed to create debug utils messenger!");

  return debug_messenger;
}
#endif

void VulkanWrapper::createUniformBuffers()
{
  BufferInfo   buffer_info{ .size       = sizeof(UniformBufferObject),
                            .usage      = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            .properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };
  VkDeviceSize buffer_size = sizeof(UniformBufferObject);
  for (uint8_t i = 0; i < max_frames_in_flight; ++i) {
    uniform_buffers_[i] = Buffer(&device_, buffer_info);
    vkMapMemory(device_.logical(), uniform_buffers_[i].memory(), 0, buffer_size,
                0, &uniform_buffers_mapped_[i]);
  }
}

void VulkanWrapper::createCommandBuffers()
{
  VkCommandBufferAllocateInfo alloc_info{};
  alloc_info.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = command_pool_.get();
  alloc_info.level       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = max_frames_in_flight;

  if (vkAllocateCommandBuffers(device_.logical(), &alloc_info,
                               command_buffers_.data()) != VK_SUCCESS)
    ThrowVk("Failed to create command buffers!");
}

void VulkanWrapper::createDescriptorSets()
{
  std::vector<VkDescriptorSetLayout> layouts(max_frames_in_flight,
                                             descriptor_set_layout_.get());

  VkDescriptorSetAllocateInfo alloc_info{};
  alloc_info.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc_info.descriptorPool = descriptor_pool_.get();
  alloc_info.descriptorSetCount = static_cast<uint32_t>(max_frames_in_flight);
  alloc_info.pSetLayouts        = layouts.data();

  if (vkAllocateDescriptorSets(device_.logical(), &alloc_info,
                               descriptor_sets_.data()) != VK_SUCCESS)
    ThrowVk("Failed to allocate descriptor sets!");

  for (size_t i = 0; i < max_frames_in_flight; ++i) {
    VkDescriptorBufferInfo buffer_info{};
    buffer_info.buffer = uniform_buffers_[i].get();
    buffer_info.offset = 0;
    buffer_info.range  = sizeof(UniformBufferObject);

    VkDescriptorImageInfo image_info{};
    image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image_info.imageView   = texture_sampler_.texture.view().get();
    image_info.sampler     = texture_sampler_.sampler.get();

    std::array<VkWriteDescriptorSet, 2> descriptor_writes{};
    descriptor_writes[0].sType      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_writes[0].dstSet     = descriptor_sets_[i];
    descriptor_writes[0].dstBinding = 0;
    descriptor_writes[0].dstArrayElement = 0;
    descriptor_writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_writes[0].descriptorCount = 1;
    // Either pBufferInfo or pImageInfo or pTexelBufferView is set
    descriptor_writes[0].pBufferInfo      = &buffer_info;
    descriptor_writes[0].pImageInfo       = nullptr;
    descriptor_writes[0].pTexelBufferView = nullptr;

    descriptor_writes[1].sType      = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_writes[1].dstSet     = descriptor_sets_[i];
    descriptor_writes[1].dstBinding = 1;
    descriptor_writes[1].dstArrayElement = 0;
    descriptor_writes[1].descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptor_writes[1].descriptorCount = 1;
    // Either pBufferInfo or pImageInfo or pTexelBufferView is set
    descriptor_writes[1].pBufferInfo      = nullptr;
    descriptor_writes[1].pImageInfo       = &image_info;
    descriptor_writes[1].pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(device_.logical(),
                           static_cast<uint32_t>(descriptor_writes.size()),
                           descriptor_writes.data(), 0, nullptr);
  }
}
void VulkanWrapper::record(uint32_t image_index)
{
  VkCommandBufferBeginInfo command_buffer_info{};
  command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  command_buffer_info.flags = 0;
  command_buffer_info.pInheritanceInfo = nullptr;

  if (vkBeginCommandBuffer(command_buffers_[current_frame_],
                           &command_buffer_info) != VK_SUCCESS)
    ThrowVk("Failed to begin command buffer!");

  VkRenderPassBeginInfo render_pass_info{};
  render_pass_info.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  render_pass_info.renderPass        = render_pass_.get();
  render_pass_info.framebuffer       = swapchain_.framebuffer(image_index);
  render_pass_info.renderArea.offset = { 0, 0 };
  render_pass_info.renderArea.extent = swapchain_.extent();

  VkClearValue clear_color         = { { { 0.0f, 0.0f, 0.0f, 1.0f } } };
  render_pass_info.clearValueCount = 1;
  render_pass_info.pClearValues    = &clear_color;

  vkCmdBeginRenderPass(command_buffers_[current_frame_], &render_pass_info,
                       VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(command_buffers_[current_frame_],
                    VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.get());
  VkViewport viewport{};
  viewport.x        = 0.0f;
  viewport.y        = 0.0f;
  viewport.width    = static_cast<float>(swapchain_.extent().width);
  viewport.height   = static_cast<float>(swapchain_.extent().height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(command_buffers_[current_frame_], 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = { 0, 0 };
  scissor.extent = swapchain_.extent();
  vkCmdSetScissor(command_buffers_[current_frame_], 0, 1, &scissor);

  vkCmdBindPipeline(command_buffers_[current_frame_],
                    VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.get());

  VkBuffer     vertex_buffers[] = { vertex_buffer_.get() };
  VkDeviceSize offsets[]        = { 0 };

  vkCmdBindVertexBuffers(command_buffers_[current_frame_], 0, 1, vertex_buffers,
                         offsets);
  vkCmdBindIndexBuffer(command_buffers_[current_frame_], index_buffer_.get(), 0,
                       VK_INDEX_TYPE_UINT16);

  vkCmdBindDescriptorSets(command_buffers_[current_frame_],
                          VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.layout(),
                          0, 1, &descriptor_sets_[current_frame_], 0, nullptr);

  vkCmdDrawIndexed(command_buffers_[current_frame_],
                   static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

  vkCmdEndRenderPass(command_buffers_[current_frame_]);
  if (vkEndCommandBuffer(command_buffers_[current_frame_]) != VK_SUCCESS)
    ThrowVk("Failed to record command buffer!");
}

ELDR_IMPORT_CORE_TYPES();
void VulkanWrapper::updateUniformBuffer(uint32_t current_image)
{
  static auto start_time   = std::chrono::high_resolution_clock::now();
  auto        current_time = std::chrono::high_resolution_clock::now();
  float       time = std::chrono::duration<float, std::chrono::seconds::period>(
                 current_time - start_time)
                 .count();

  UniformBufferObject ubo{};
  ubo.model = glm::rotate(Mat4f(1.0f), time * glm::radians(90.0f),
                          Vec3f(0.0f, 0.0f, 1.0f));
  ubo.view  = glm::lookAt(Vec3f(2.0f, 2.0f, 2.0f), Vec3f(0.0f, 0.0f, 0.0f),
                          Vec3f(0.0f, 0.0f, 1.0f));
  ubo.proj  = glm::perspective(glm::radians(45.0f),
                               swapchain_.extent().width /
                                 (float) swapchain_.extent().height,
                               0.1f, 10.0f);
  ubo.proj[1][1] *= -1;
  memcpy(uniform_buffers_mapped_[current_image], &ubo, sizeof(ubo));
}

void VulkanWrapper::drawFrame()
{
  vkWaitForFences(device_.logical(), 1,
                  &in_flight_fences_[current_frame_].get(), VK_TRUE,
                  UINT64_MAX);
  vkResetFences(device_.logical(), 1, &in_flight_fences_[current_frame_].get());

  uint32_t image_index;
  vkAcquireNextImageKHR(device_.logical(), swapchain_.get(), UINT64_MAX,
                        image_available_sem_[current_frame_].get(),
                        VK_NULL_HANDLE, &image_index);
  vkResetCommandBuffer(command_buffers_[current_frame_], 0);

  record(image_index);

  updateUniformBuffer(current_frame_);

  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore wait_semaphores[] = {
    image_available_sem_[current_frame_].get()
  };
  VkPipelineStageFlags wait_stages[] = {
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
  };
  submit_info.waitSemaphoreCount  = 1;
  submit_info.pWaitSemaphores     = wait_semaphores;
  submit_info.pWaitDstStageMask   = wait_stages;
  submit_info.commandBufferCount  = 1;
  submit_info.pCommandBuffers     = &command_buffers_[current_frame_];
  VkSemaphore signal_semaphores[] = {
    render_finished_sem_[current_frame_].get()
  };
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores    = signal_semaphores;

  if (vkQueueSubmit(device_.graphicsQueue(), 1, &submit_info,
                    in_flight_fences_[current_frame_].get()) != VK_SUCCESS)
    ThrowVk("Failed to submit draw command buffer!");

  VkPresentInfoKHR present_info{};
  present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores    = signal_semaphores;

  VkSwapchainKHR swapchains[] = { swapchain_.get() };
  present_info.swapchainCount = 1;
  present_info.pSwapchains    = swapchains;
  present_info.pImageIndices  = &image_index;

  vkQueuePresentKHR(device_.presentQueue(), &present_info);

  current_frame_ = (current_frame_ + 1) % max_frames_in_flight;
}

} // namespace vk
} // namespace eldr
