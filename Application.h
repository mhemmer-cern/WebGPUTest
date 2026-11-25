#pragma once

#include "glm/fwd.hpp"
#include <webgpu/webgpu_cpp.h>

#include <glm/glm.hpp> // all types inspired from GLSL

// Forward-declare
struct GLFWwindow;

class Application {
public:
  // Initialize everything and return true if it went all right
  bool Initialize();

  // Uninitialize everything that was initialized
  void Terminate();

  // Draw a frame and handle events
  void MainLoop();

  // Return true as long as the main loop should keep on running
  bool IsRunning();

private:

  /**
  * The same structure as in the shader, replicated in C++
  */
  struct MyUniforms {
    glm::mat4x4 projectionMatrix;
    glm::mat4x4 viewMatrix;
    glm::mat4x4 modelMatrix;
    glm::vec4 color; 
    float time;
    float _pad[3];
  };

  // Have the compiler check byte alignment
  static_assert(sizeof(MyUniforms) % 16 == 0);

  /**
     * @brief Create the render pipeline.
     *
     * @pre  mDevice is valid and the surface is configured
     *       (mSurfaceFormat is set), i.e. call this late in Initialize().
     * @post mPipeline is valid if the function returned true.
     *
     * @return false if the shader could not be loaded or the pipeline
     *         could not be created.
     * @see InitializeBuffers()
     */
  bool InitializePipeline();

  /**
     * @brief Create the vertex buffer and upload the triangle's vertices.
     *
     * @pre  mDevice and mQueue are valid.
     * @post mVertexBuffer holds mVertexCount vertices, laid out as
     *       expected by the pipeline's vertex buffer layout.
     *
     * @note Independent of InitializePipeline(); the two only meet at draw
     *       time via SetPipeline() and SetVertexBuffer().
     */
  bool InitializeBuffers();

  /**
    * @brief Create bind groups
    */
  void InitializeBindGroups();

  // Get next texture view of the surface
  wgpu::TextureView GetNextSurfaceView();

  // We put here all the variables that are shared between init and main loop
  GLFWwindow* mWindow = nullptr;
  wgpu::Instance mInstance = nullptr;
  wgpu::Device mDevice = nullptr;
  wgpu::Queue mQueue = nullptr;
  wgpu::Surface mSurface = nullptr;
  wgpu::RenderPipeline mPipeline = nullptr;
  wgpu::TextureFormat mSurfaceFormat = wgpu::TextureFormat::Undefined;
  uint32_t mPointCount{};
  uint32_t mIndexCount{};
  wgpu::Buffer mPointBuffer = nullptr;
  wgpu::Buffer mIndexBuffer = nullptr;
  wgpu::Buffer mUniformBuffer = nullptr;

  wgpu::PipelineLayout mLayout = nullptr;
  wgpu::BindGroupLayout mBindGroupLayout = nullptr;
  wgpu::BindGroup mBindGroup = nullptr;
  uint32_t mUniformStride{};
  wgpu::TextureFormat mDepthTextureFormat = wgpu::TextureFormat::Depth24Plus;
  wgpu::TextureView mDepthTextureView = nullptr;

  MyUniforms mUniforms{};
  glm::mat4x4 mS{1.0f};   // scale of the model
  glm::mat4x4 mT1{1.0f};  // translation of the model
};
