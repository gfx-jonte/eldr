#include <eldr/core/fwd.hpp>
#include <eldr/core/logger.hpp>
#include <eldr/core/math.hpp>
#include <eldr/vulkan/common.hpp>
#include <eldr/vulkan/helpers.hpp>
#include <eldr/vulkan/vertex.hpp>
#include <eldr/vulkan/vulkan.hpp>

#include <backends/imgui_impl_vulkan.h>

#include <string>
#include <unordered_map>

namespace eldr {
namespace vk {
struct UniformBufferObject {
  ELDR_IMPORT_CORE_TYPES();
  alignas(16) Mat4f model;
  alignas(16) Mat4f view;
  alignas(16) Mat4f proj;
};

// fwd
// -----------------------------------------------------------------------------
#ifdef ELDR_VULKAN_DEBUG_REPORT
static VkDebugUtilsMessengerEXT
setupDebugMessenger(VkInstance& instance, VkAllocationCallbacks* allocator);
#endif
// -----------------------------------------------------------------------------
VulkanEngine::VulkanEngine(GLFWwindow* const        window,
                           std::vector<const char*> instance_extensions)
  : window_(window), current_frame_(0), instance_(instance_extensions),
#ifdef ELDR_VULKAN_DEBUG_REPORT
    debug_messenger_(setupDebugMessenger(instance_.get(), nullptr)),
#endif
    surface_(&instance_, window), device_(instance_, surface_),
    swapchain_(&device_, surface_, window),
    // render_pass_(&device_, swapchain_.format()),
    descriptor_set_layout_(&device_),
    descriptor_pool_(&device_,
                     { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                         static_cast<uint32_t>(max_frames_in_flight) },
                       { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                         static_cast<uint32_t>(max_frames_in_flight) } },
                     static_cast<uint32_t>(max_frames_in_flight)),
    pipeline_(&device_, swapchain_, swapchain_.render_pass_,
              descriptor_set_layout_, swapchain_.msaaSamples()),
    command_pool_(&device_, surface_),
    texture_sampler_(&device_, command_pool_), vertices_(), indices_(),
    vertex_buffer_(&device_, vertices_, command_pool_),
    index_buffer_(&device_, indices_, command_pool_),
    uniform_buffers_(max_frames_in_flight),
    uniform_buffers_mapped_(max_frames_in_flight),
    command_buffers_(max_frames_in_flight),
    // command_buffers_(max_frames_in_flight),
    descriptor_sets_(max_frames_in_flight)
{

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

VulkanEngine::~VulkanEngine()
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

void VulkanEngine::createUniformBuffers()
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

void VulkanEngine::createCommandBuffers()
{
  for (uint32_t i = 0; i < max_frames_in_flight; ++i) {
    command_buffers_[i] = CommandBuffer(&device_, &command_pool_);
  }
}

void VulkanEngine::createDescriptorSets()
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

// Needed only for imgui thing
// TODO: this can be improved...
static void checkVkResult(VkResult result) { CheckVkResult(result); }

void VulkanEngine::initImGui()
{

  // VkDescriptorPoolSize pool_sizes[] = {
  //   { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
  // };
  // VkDescriptorPoolCreateInfo pool_info = {};
  // pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  // pool_info.flags         =
  // VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; pool_info.maxSets = 1;
  // pool_info.poolSizeCount = (uint32_t) IM_ARRAYSIZE(pool_sizes);
  // pool_info.pPoolSizes    = pool_sizes;
  // VkResult err = vkCreateDescriptorPool(device_.logical(), &pool_info,
  // nullptr,
  //                                       &imgui_descriptor_pool_);
  // CheckVkResult(err);

  // TODO: make general DescriptorPool class

  const std::vector<VkDescriptorPoolSize> pool_sizes = {
    { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
  };

  DescriptorPool imgui_descriptor_pool(
    &device_, pool_sizes, static_cast<uint32_t>(max_frames_in_flight));

  QueueFamilyIndices queue_family_indices =
    findQueueFamilies(device_.physical(), surface_.get());
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance                  = instance_.get();
  init_info.PhysicalDevice            = device_.physical();
  init_info.Device                    = device_.logical();
  init_info.QueueFamily     = queue_family_indices.graphics_family.value();
  init_info.Queue           = device_.graphicsQueue();
  init_info.PipelineCache   = VK_NULL_HANDLE;
  init_info.DescriptorPool  = imgui_descriptor_pool.get();
  init_info.Subpass         = 0;
  init_info.MinImageCount   = swapchain_.minImageCount();
  init_info.ImageCount      = swapchain_.minImageCount();
  init_info.MSAASamples     = swapchain_.msaaSamples();
  init_info.Allocator       = nullptr;
  init_info.CheckVkResultFn = vk::checkVkResult;
  ImGui_ImplVulkan_Init(&init_info, swapchain_.render_pass_.get());

  // Upload fonts
  {
    CommandBuffer cb(&device_, &command_pool_);
    cb.beginSingleCommand();
    ImGui_ImplVulkan_CreateFontsTexture(cb.get());
    //cb.end();
    cb.submit();
    ImGui_ImplVulkan_DestroyFontUploadObjects();
  }
}

void VulkanEngine::record(uint32_t image_index)
{
  command_buffers_[current_frame_].begin();

  VkCommandBuffer current_cb = command_buffers_[current_frame_].get();

  // Note that the order matters, depth is on index 1
  std::array<VkClearValue, 2> clear_values{};
  clear_values[0].color        = { { 0.0f, 0.0f, 0.0f, 1.0f } };
  clear_values[1].depthStencil = { 1.0f, 0 };

  command_buffers_[current_frame_].beginRenderPass(
    swapchain_.render_pass_.get(), swapchain_.framebuffers_[image_index],
    swapchain_.extent(), clear_values.size(), clear_values.data());

  vkCmdBindPipeline(current_cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline_.get());
  VkViewport viewport{};
  viewport.x        = 0.0f;
  viewport.y        = 0.0f;
  viewport.width    = static_cast<float>(swapchain_.extent().width);
  viewport.height   = static_cast<float>(swapchain_.extent().height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(current_cb, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = { 0, 0 };
  scissor.extent = swapchain_.extent();
  vkCmdSetScissor(current_cb, 0, 1, &scissor);

  vkCmdBindPipeline(current_cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline_.get());

  VkBuffer     vertex_buffers[] = { vertex_buffer_.get() };
  VkDeviceSize offsets[]        = { 0 };

  vkCmdBindVertexBuffers(current_cb, 0, 1, vertex_buffers, offsets);
  vkCmdBindIndexBuffer(current_cb, index_buffer_.get(), 0,
                       VK_INDEX_TYPE_UINT32);

  vkCmdBindDescriptorSets(current_cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline_.layout(), 0, 1,
                          &descriptor_sets_[current_frame_], 0, nullptr);

  vkCmdDrawIndexed(current_cb, static_cast<uint32_t>(indices_.size()), 1, 0, 0,
                   0);

  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), current_cb);

  command_buffers_[current_frame_].endRenderPass();
  command_buffers_[current_frame_].end();
}

void VulkanEngine::updateUniformBuffer(uint32_t current_image)
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

void VulkanEngine::submitGeometry(const std::vector<Vec3f>& positions,
                                  const std::vector<Vec2f>& texcoords)
{
  vertices_.clear();
  indices_.clear();
  std::unordered_map<Vertex, uint32_t> unique_vertices{};

  for (uint32_t i = 0; i < positions.size(); ++i) {
    Vertex v{ positions[i], { 1.0f, 1.0f, 1.0f }, texcoords[i] };
    if (unique_vertices.count(v) == 0) {
      unique_vertices[v] = static_cast<uint32_t>(vertices_.size());
      vertices_.push_back(v);
    }
    indices_.push_back(unique_vertices[v]);
  }
  vertex_buffer_ = Buffer(&device_, vertices_, command_pool_);
  index_buffer_  = Buffer(&device_, indices_, command_pool_);
}

void VulkanEngine::drawFrame()
{
  vkWaitForFences(device_.logical(), 1,
                  &in_flight_fences_[current_frame_].get(), VK_TRUE,
                  UINT64_MAX);

  uint32_t image_index;
  VkResult result = vkAcquireNextImageKHR(
    device_.logical(), swapchain_.get(), UINT64_MAX,
    image_available_sem_[current_frame_].get(), VK_NULL_HANDLE, &image_index);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    swapchain_.recreate(surface_, window_);
    return;
  }
  else if (result != VK_SUCCESS) {
    ThrowVk("Failed to acquire swapchain image!");
  }

  vkResetFences(device_.logical(), 1, &in_flight_fences_[current_frame_].get());

  command_buffers_[current_frame_].reset();

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
  submit_info.pCommandBuffers     = &command_buffers_[current_frame_].get();
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

  result = vkQueuePresentKHR(device_.presentQueue(), &present_info);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      framebuffer_resized_) {
    framebuffer_resized_ = false;
    swapchain_.recreate(surface_, window_);
    return;
  }
  else if (result != VK_SUCCESS) {
    ThrowVk("Failed to present swapchain image!");
  }

  current_frame_ = (current_frame_ + 1) % max_frames_in_flight;
}

} // namespace vk
} // namespace eldr
