#include <eldr/app.hpp>
#include <eldr/render/mesh.hpp>
#include <eldr/vulkan/vulkan.hpp>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

namespace eldr {

// -----------------------------------------------------------------------------
// fwd
// -----------------------------------------------------------------------------
static void                     setupImGui(GLFWwindow*, vk::VulkanEngine*);
static GLFWwindow*              initGLFWwindow(uint32_t width, uint32_t height);
static std::vector<const char*> getGLFWextensions();
// -----------------------------------------------------------------------------

static void glfwErrorCallback(int error, const char* description)
{
  spdlog::error("GLFW Error %d: %s", error, description);
}

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
  // Let vulkan wrapper manage resize
  (void) width;
  (void) height;
  auto app = reinterpret_cast<EldrApp*>(glfwGetWindowUserPointer(window));
  app->resize();
}

EldrApp::EldrApp()
  : window_(initGLFWwindow(width, height)),
    vk_engine_(new vk::VulkanEngine(window_, getGLFWextensions())),
    scene_({ model_path_str, texture_path_str })
{

  glfwSetWindowUserPointer(window_, this);
  glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
  setupImGui(window_, vk_engine_);
}

EldrApp::~EldrApp()
{
  glfwDestroyWindow(window_);
  glfwTerminate();
  delete vk_engine_;
}

static GLFWwindow* initGLFWwindow(uint32_t width, uint32_t height)
{
  // Initialize GLFW
  glfwSetErrorCallback(glfwErrorCallback);
  if (!glfwInit()) {
    Throw("[GLFW]: Failed to initialize");
  }

  // Vulkan pre-check
  if (!glfwVulkanSupported()) {
    Throw("[GLFW]: Vulkan not supported");
  }

  // Create GLFW Window with Vulkan context
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow* window =
    glfwCreateWindow(width, height, "Eldr", nullptr, nullptr);

  return window;
}

static std::vector<const char*> getGLFWextensions()
{
  std::vector<const char*> extensions{};
  uint32_t                 extensions_count = 0;
  const char**             glfw_extensions =
    glfwGetRequiredInstanceExtensions(&extensions_count);
  for (size_t i = 0; i < extensions_count; i++)
    extensions.push_back(glfw_extensions[i]);
  return extensions;
}

static void setupImGui(GLFWwindow* window, vk::VulkanEngine* vk_engine)
{
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void) io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.Fonts->AddFontDefault();
  ImGui::StyleColorsDark();
  // TODO: set up fonts and more style stuff
  ImGui_ImplGlfw_InitForVulkan(window, true);
  vk_engine->initImGui();
}

void EldrApp::resize() const { vk_engine_->framebuffer_resized_ = true; }

void EldrApp::run()
{
  bool show_demo_window = true;
  submitGeometry(scene_.getShapes());
  while (!glfwWindowShouldClose(window_)) {
    glfwPollEvents();
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow(&show_demo_window);
    //ImGuiIO& io = ImGui::GetIO();
    ImGui::Render();
    vk_engine_->drawFrame();
    //if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    //  ImGui::UpdatePlatformWindows();
    //  ImGui::RenderPlatformWindowsDefault();
    //}
  }
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void EldrApp::submitGeometry(const std::vector<Shape*>& shapes)
{
  // Create one big vertex buffer along with index buffer
  // TODO: Shape should contain texture ref probably
  Mesh* mesh = dynamic_cast<Mesh*>(shapes[0]);
  if (mesh != nullptr) {
    vk_engine_->submitGeometry(mesh->vertexPositions(),
                               mesh->vertexTexCoords());
  }
}
} // Namespace eldr
