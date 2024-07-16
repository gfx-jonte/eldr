#include <eldr/core/logger.hpp>
#include <eldr/vulkan/pipeline.hpp>
#include <eldr/vulkan/vertex.hpp>

#include <fstream>
#include <string>

namespace eldr {
namespace vk {
// fwd -------------------------------------------------------------------------
static std::vector<char> loadShader(const std::string& type);
VkShaderModule           createShaderModule(VkDevice                 device,
                                            const std::vector<char>& bytecode);
// -----------------------------------------------------------------------------
Pipeline::Pipeline(const Device* device, const Swapchain& swapchain,
                   const RenderPass&          render_pass,
                   const DescriptorSetLayout& descriptor_set_layout,
                   VkSampleCountFlagBits      num_samples)
  : device_(device)
{
  std::vector<char> vert_shader = loadShader("vertex");
  std::vector<char> frag_shader = loadShader("fragment");
  spdlog::info("Vertex shader size = {}", vert_shader.size());
  spdlog::info("Fragment shader size = {}", frag_shader.size());

  VkShaderModule vert_shader_module =
    createShaderModule(device_->logical(), vert_shader);
  VkShaderModule frag_shader_module =
    createShaderModule(device_->logical(), frag_shader);

  VkPipelineShaderStageCreateInfo vert_shader_stage_ci{};
  vert_shader_stage_ci.sType =
    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vert_shader_stage_ci.stage  = VK_SHADER_STAGE_VERTEX_BIT;
  vert_shader_stage_ci.module = vert_shader_module;
  vert_shader_stage_ci.pName  = "main";
  // The variable below is useful for constants in the shader. Can be more
  // efficient than configuring constants at render time.
  vert_shader_stage_ci.pSpecializationInfo = nullptr;

  VkPipelineShaderStageCreateInfo frag_shader_stage_ci{};
  frag_shader_stage_ci.sType =
    VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  frag_shader_stage_ci.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
  frag_shader_stage_ci.module = frag_shader_module;
  frag_shader_stage_ci.pName  = "main";
  // The variable below is useful for constants in the shader. Can be more
  // efficient than configuring constants at render time.
  frag_shader_stage_ci.pSpecializationInfo = nullptr;

  VkPipelineShaderStageCreateInfo shader_stages[] = { vert_shader_stage_ci,
                                                      frag_shader_stage_ci };

  VkPipelineVertexInputStateCreateInfo vertex_input_info{};
  auto binding_description    = Vertex::getBindingDescription();
  auto attribute_descriptions = Vertex::getAttributeDescriptions();
  vertex_input_info.sType =
    VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertex_input_info.vertexBindingDescriptionCount = 1;
  vertex_input_info.pVertexBindingDescriptions    = &binding_description;
  vertex_input_info.vertexAttributeDescriptionCount =
    static_cast<uint32_t>(attribute_descriptions.size());
  vertex_input_info.pVertexAttributeDescriptions =
    attribute_descriptions.data();

  VkPipelineInputAssemblyStateCreateInfo input_assembly{};
  input_assembly.sType =
    VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport{};
  viewport.x        = 0.0f;
  viewport.y        = 0.0f;
  viewport.width    = (float) swapchain.extent().width;
  viewport.height   = (float) swapchain.extent().height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = { 0, 0 };
  scissor.extent = swapchain.extent();

  std::vector<VkDynamicState> dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT,
                                                 VK_DYNAMIC_STATE_SCISSOR };

  VkPipelineDynamicStateCreateInfo dynamic_state{};
  dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamic_state.dynamicStateCount =
    static_cast<uint32_t>(dynamic_states.size());
  dynamic_state.pDynamicStates = dynamic_states.data();

  VkPipelineViewportStateCreateInfo viewport_state{};
  viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state.viewportCount = 1;
  viewport_state.pViewports    = &viewport;
  viewport_state.scissorCount  = 1;
  viewport_state.pScissors     = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable        = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth               = 1.0f;
  rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable         = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType =
    VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable   = VK_TRUE;
  multisampling.rasterizationSamples  = num_samples;
  multisampling.minSampleShading      = 0.2f;
  multisampling.pSampleMask           = nullptr;  // Optional
  multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
  multisampling.alphaToOneEnable      = VK_FALSE; // Optional

  VkPipelineDepthStencilStateCreateInfo depth_stencil{};
  depth_stencil.sType =
    VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depth_stencil.depthTestEnable       = VK_TRUE;
  depth_stencil.depthWriteEnable      = VK_TRUE;
  depth_stencil.depthCompareOp        = VK_COMPARE_OP_LESS;
  depth_stencil.depthBoundsTestEnable = VK_FALSE;
  depth_stencil.minDepthBounds        = 0.0f; // optional
  depth_stencil.maxDepthBounds        = 1.0f; // optional
  // Stencil buffer ops not used. Format of depth image must contain a stencil
  // component if this is to be used.
  depth_stencil.stencilTestEnable = VK_FALSE;
  depth_stencil.front             = {};
  depth_stencil.back              = {};

  // TODO: may want to change
  VkPipelineColorBlendAttachmentState color_blend_attachment{};
  color_blend_attachment.colorWriteMask =
    VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
    VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color_blend_attachment.blendEnable         = VK_TRUE;
  color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  color_blend_attachment.dstColorBlendFactor =
    VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  color_blend_attachment.colorBlendOp        = VK_BLEND_OP_ADD;
  color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  color_blend_attachment.alphaBlendOp        = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo color_blending{};
  color_blending.sType =
    VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blending.logicOpEnable     = VK_FALSE;
  color_blending.logicOp           = VK_LOGIC_OP_COPY; // Optional
  color_blending.attachmentCount   = 1;
  color_blending.pAttachments      = &color_blend_attachment;
  color_blending.blendConstants[0] = 0.0f; // Optional
  color_blending.blendConstants[1] = 0.0f; // Optional
  color_blending.blendConstants[2] = 0.0f; // Optional
  color_blending.blendConstants[3] = 0.0f; // Optional

  VkPipelineLayoutCreateInfo pipeline_layout_ci{};
  pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_ci.setLayoutCount         = 1;
  pipeline_layout_ci.pSetLayouts            = &descriptor_set_layout.get();
  pipeline_layout_ci.pushConstantRangeCount = 0;       // Optional
  pipeline_layout_ci.pPushConstantRanges    = nullptr; // Optional

  if (vkCreatePipelineLayout(device_->logical(), &pipeline_layout_ci, nullptr,
                             &layout_) != VK_SUCCESS) {
    ThrowVk("Failed to create pipeline layout");
  }

  VkGraphicsPipelineCreateInfo pipeline_ci{};
  pipeline_ci.sType      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_ci.stageCount = 2;
  pipeline_ci.pStages    = shader_stages;
  pipeline_ci.pVertexInputState   = &vertex_input_info;
  pipeline_ci.pInputAssemblyState = &input_assembly;
  pipeline_ci.pViewportState      = &viewport_state;
  pipeline_ci.pRasterizationState = &rasterizer;
  pipeline_ci.pMultisampleState   = &multisampling;
  pipeline_ci.pDepthStencilState  = &depth_stencil;
  pipeline_ci.pColorBlendState    = &color_blending;
  pipeline_ci.pDynamicState       = &dynamic_state;
  pipeline_ci.layout              = layout_;
  pipeline_ci.renderPass          = render_pass.get();
  pipeline_ci.subpass             = 0;
  pipeline_ci.basePipelineHandle  = VK_NULL_HANDLE; // Optional
  pipeline_ci.basePipelineIndex   = -1;             // Optional

  if (vkCreateGraphicsPipelines(device_->logical(), VK_NULL_HANDLE, 1,
                                &pipeline_ci, nullptr,
                                &pipeline_) != VK_SUCCESS) {
    ThrowVk("Failed to create graphics pipeline!");
  }

  vkDestroyShaderModule(device_->logical(), vert_shader_module, nullptr);
  vkDestroyShaderModule(device_->logical(), frag_shader_module, nullptr);
}

Pipeline::~Pipeline()
{
  if (pipeline_ != VK_NULL_HANDLE)
    vkDestroyPipeline(device_->logical(), pipeline_, nullptr);
  if (layout_ != VK_NULL_HANDLE)
    vkDestroyPipelineLayout(device_->logical(), layout_, nullptr);
}

static std::vector<char> loadShader(const std::string& type)
{
  const char* env_p = std::getenv("ELDR_DIR");
  if (env_p == nullptr) {
    Throw("loadShader(): Environment not set up correctly");
  }

  std::string filename{};
  if (type == "vertex")
    filename = std::string(env_p) + "/shaders/vert.spv";
  else if (type == "fragment")
    filename = std::string(env_p) + "/shaders/frag.spv";

  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    Throw("loadShader(): Failed to open file!");
  }

  size_t            file_size = (size_t) file.tellg();
  std::vector<char> buffer(file_size);
  file.seekg(0);
  file.read(buffer.data(), file_size);
  file.close();
  return buffer;
}

VkShaderModule createShaderModule(VkDevice                 device,
                                  const std::vector<char>& bytecode)
{
  VkShaderModuleCreateInfo shader_ci{};
  shader_ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  shader_ci.codeSize = bytecode.size();
  shader_ci.pCode    = reinterpret_cast<const uint32_t*>(bytecode.data());

  VkShaderModule shader_module;
  if (vkCreateShaderModule(device, &shader_ci, nullptr, &shader_module) !=
      VK_SUCCESS) {
    ThrowVk("Failed to create shader module!");
  }
  return shader_module;
}
} // namespace vk
} // namespace eldr
