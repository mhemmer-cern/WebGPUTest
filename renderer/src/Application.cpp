#include "Renderer/Application.h"
#include "Renderer/ResourceManager.h"
#include "Renderer/VertexAttribute.h"
#include "Renderer/webgpu-utils.h"

#include <GLFW/glfw3.h>

#include <glm/ext.hpp>
#include <glm/glm.hpp> // all types inspired from GLSL

#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_cpp_print.h>
#include <webgpu/webgpu_glfw.h>

// #include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace wgpu;
using namespace glm;
// constexpr float PI = 3.14159265358979323846f;

bool Application::Initialize() {

  // init GLFW
  if (!glfwInit()) {
    std::cerr << "Could not initialize GLFW!" << std::endl;
    return false;
  }

  // Create the window
  glfwWindowHint(GLFW_CLIENT_API,
                 GLFW_NO_API); // <-- extra info for glfwCreateWindow
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  mWindow = glfwCreateWindow(640, 480, "Learn WebGPU", nullptr, nullptr);

  if (!mWindow) {
    std::cerr << "Could not open window!" << std::endl;
    glfwTerminate();
    return false;
  }

  // We create a descriptor
  static const auto kTimedWaitAny = InstanceFeatureName::TimedWaitAny;
  InstanceDescriptor desc = {};

  desc.requiredFeatureCount = 1;
  desc.requiredFeatures = &kTimedWaitAny;

  mInstance = CreateInstance(&desc);
  // We can check whether there is actually an instance created
  if (!mInstance) {
    std::cerr << "Could not initialize WebGPU!" << std::endl;
    return false;
  }

  // Display the object (WGPUInstance is a simple pointer, it may be
  // copied around without worrying about its size).
  std::cout << "WGPU instance: " << mInstance.Get() << std::endl;

  mSurface = glfw::CreateSurfaceForWindow(mInstance, mWindow);

  std::cout << "Requesting adapter..." << std::endl;

  RequestAdapterOptions adapterOpts{};
  adapterOpts.compatibleSurface = mSurface;

  Adapter adapter = requestAdapterSync(mInstance, adapterOpts);

  std::cout << "Got adapter: " << adapter.Get() << std::endl;
  inspectAdapter(adapter);

  std::cout << "Requesting device..." << std::endl;

  DeviceDescriptor deviceDesc{};
  // Any name works here, that's your call
  deviceDesc.label = "My Device";
  std::vector<FeatureName> features;
  // No required feature for now
  deviceDesc.requiredFeatureCount = features.size();
  deviceDesc.requiredFeatures = features.data();
  // Make sure 'features' lives until the call to wgpuAdapterRequestDevice!
  Limits requiredLimits{};
  requiredLimits.maxVertexAttributes = 3;
  requiredLimits.maxUniformBufferBindingSize = 16 * sizeof(VertexAttributes);
  requiredLimits.maxVertexBufferArrayStride = sizeof(VertexAttributes);
  requiredLimits.maxInterStageShaderVariables = 6;
  deviceDesc.requiredLimits = &requiredLimits;
  // Make sure that the 'requiredLimits' variable lives until the call to
  // wgpuAdapterRequestDevice!
  deviceDesc.defaultQueue.label = "The Default Queue";
  deviceDesc.SetDeviceLostCallback(
      CallbackMode::AllowSpontaneous,
      [](Device const &device, DeviceLostReason reason, StringView message) {
        std::cout << "Device " << device.Get() << " was lost: reason " << reason
                  << " (" << std::string_view(message) << ")" << std::endl;
      });

  deviceDesc.SetUncapturedErrorCallback([](Device const &device, ErrorType type,
                                           StringView message) {
    std::cout << "Uncaptured error in device " << device.Get() << ": type "
              << type << " (" << std::string_view(message) << ")" << std::endl;
  });

  mDevice = requestDeviceSync(mInstance, adapter, deviceDesc);
  if (!mDevice) {
    return false;
  }
  std::cout << "Got device: " << mDevice.Get() << std::endl;
  inspectDevice(mDevice);

  mQueue = mDevice.GetQueue();

  // Ask which formats this surface supports with this adapter
  SurfaceCapabilities capabilities;
  if (mSurface.GetCapabilities(adapter, &capabilities) != Status::Success) {
    return false;
  }

  SurfaceConfiguration config{};
  config.device = mDevice;
  config.width = 640;
  config.height = 480;
  config.format =
      capabilities.formats[0]; // the first format is the preferred one
  config.presentMode = PresentMode::Fifo;
  config.alphaMode = CompositeAlphaMode::Auto;
  mSurface.Configure(&config);
  mSurfaceFormat = config.format;

  // Create the depth texture
  TextureDescriptor depthTextureDesc{};
  depthTextureDesc.label = StringView("Z Buffer");
  depthTextureDesc.usage = TextureUsage::RenderAttachment;
  depthTextureDesc.size = {640, 480, 1};
  depthTextureDesc.format = mDepthTextureFormat;
  Texture depthTexture = mDevice.CreateTexture(&depthTextureDesc);

  // Create the view of the depth texture manipulated by the rasterizer
  mDepthTextureView = depthTexture.CreateView();

  // We can now release the texture and only hold to the view
  depthTexture = nullptr;

  if (!InitializeBuffers()) {
    return false;
  }

  if (!InitializePipeline()) {
    return false;
  }

  InitializeBindGroups();

  return true;
}

bool Application::InitializeBuffers() {
  // At the beginning of InitializeBuffers()
  // Define data vectors, but without filling them in
  // std::vector<float> pointData;
  // std::vector<uint16_t> indexData;
  // bool success = ResourceManager::loadGeometry(
  //     RESOURCE_DIR "/models/pyramid.txt", pointData, indexData, 6);
  // if (!success) {
  //   return false;
  // }

  // mIndexCount = static_cast<uint32_t>(indexData.size());

  // // We now divide the vector size by 9 fields (x, y, z, nx, ny, nz, r, g, b).
  // mPointCount = static_cast<uint32_t>(
  //     pointData.size() / (sizeof(VertexAttributes) / sizeof(float)));
  // mIndexCount = static_cast<uint32_t>(indexData.size());

  // // Create point buffer
  // BufferDescriptor pointBufferDesc{};
  // pointBufferDesc.size = pointData.size() * sizeof(float);
  // pointBufferDesc.usage =
  //     BufferUsage::CopyDst | BufferUsage::Vertex; // Vertex usage here!
  // mPointBuffer = mDevice.CreateBuffer(&pointBufferDesc);

  // // Create index buffer
  // BufferDescriptor indexBufferDesc{};
  // indexBufferDesc.size = indexData.size() * sizeof(uint16_t);
  // indexBufferDesc.size =
  //     (indexBufferDesc.size + 3) & ~3; // round up to the next multiple of 4
  // indexBufferDesc.usage =
  //     BufferUsage::CopyDst | BufferUsage::Index; // Index usage here!
  // mIndexBuffer = mDevice.CreateBuffer(&indexBufferDesc);

  // // Upload geometry data to the buffer
  // mQueue.WriteBuffer(mPointBuffer, 0, pointData.data(), pointBufferDesc.size);
  // // resize the indexData to be multiple of 2 before copying
  // indexData.resize((indexData.size() + 1) &
  //                  ~1); // round up to the next multiple of 2
  // mQueue.WriteBuffer(mIndexBuffer, 0, indexData.data(), indexBufferDesc.size);

  std::vector<VertexAttributes> vertexData;
  std::vector<uint32_t> indexData;
  if (!ResourceManager::loadGeometryFromGltf(RESOURCE_DIR "/models/mammoth.glb", vertexData, indexData)) {
      return false;
  }

  mPointCount = static_cast<uint32_t>(vertexData.size());
  mIndexCount = static_cast<uint32_t>(indexData.size());

  BufferDescriptor pointBufferDesc{};
  pointBufferDesc.size = vertexData.size() * sizeof(VertexAttributes);
  pointBufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Vertex;
  mPointBuffer = mDevice.CreateBuffer(&pointBufferDesc);
  mQueue.WriteBuffer(mPointBuffer, 0, vertexData.data(), pointBufferDesc.size);

  BufferDescriptor indexBufferDesc{};
  indexBufferDesc.size = indexData.size() * sizeof(uint32_t); // always a multiple of 4
  indexBufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Index;
  mIndexBuffer = mDevice.CreateBuffer(&indexBufferDesc);
  mQueue.WriteBuffer(mIndexBuffer, 0, indexData.data(), indexBufferDesc.size);


  Limits deviceLimits{};
  mDevice.GetLimits(&deviceLimits);

  // Subtlety
  mUniformStride = ceilToNextMultiple(
      (uint32_t)sizeof(MyUniforms),
      (uint32_t)deviceLimits.minUniformBufferOffsetAlignment);

  // Create uniform buffer
  // The buffer will only contain 1 float with the value of uTime
  // then 3 floats left empty but needed by alignment constraints
  BufferDescriptor uniformBufferDesc{};
  // (NB: stride = sizeof(MyUniforms) + spacing)
  uniformBufferDesc.size = mUniformStride + sizeof(MyUniforms);

  // Make sure to flag the buffer as BufferUsage::Uniform
  uniformBufferDesc.usage = BufferUsage::CopyDst | BufferUsage::Uniform;

  mUniformBuffer = mDevice.CreateBuffer(&uniformBufferDesc);

  // Upload first value
  // Scale the object
  mS = scale(mat4x4(1.0), vec3(0.3f));
  // Translate the object
  mT1 = translate(mat4x4(1.0), vec3(0.5, 0.0, 0.0));
  // Rotate the object
  float angle1 = static_cast<float>(glfwGetTime());
  mat4x4 R1 = rotate(mat4x4(1.0), -angle1, vec3(0.0, 1.0, 0.0));
  mUniforms.modelMatrix = R1 * mT1 * mS;

  vec3 camera(0.0f, 0.7f, 1.9f);   // where the camera is
  vec3 target(0.0f);            // what it looks at
  vec3 up(0.0f, 1.0f, 0.0f);    // world up (Y-up)
  mUniforms.viewMatrix = glm::lookAt(camera, target, up);

  float near = 0.01f;
  float far = 100.0f;
  float ratio = 640.0f / 480.0f;
  float fov = glm::radians(30.0f);
  mUniforms.projectionMatrix = glm::perspective(fov, ratio, near, far);

  mUniforms.time = 1.0f;
  mUniforms.color = {0.0f, 1.0f, 0.4f, 0.7f};
  mQueue.WriteBuffer(mUniformBuffer, 0, &mUniforms, sizeof(mUniforms));

  // Upload second value
  mUniforms.time = -1.0f;
  mUniforms.color = {1.0f, 1.0f, 1.0f, 0.7f};
  mQueue.WriteBuffer(mUniformBuffer, mUniformStride, &mUniforms,
                     sizeof(mUniforms));

  return true;
}

void Application::InitializeBindGroups() {

  // Create a binding
  BindGroupEntry binding{};
  // The index of the binding (the entries in bindGroupDesc can be in any order)
  binding.binding = 0;
  // The buffer it is actually bound to
  binding.buffer = mUniformBuffer;
  // We can specify an offset within the buffer, so that a single buffer can
  // hold multiple uniform blocks.
  binding.offset = 0;
  // And we specify again the size of the buffer.
  binding.size = sizeof(MyUniforms);

  // A bind group contains one or multiple bindings
  BindGroupDescriptor bindGroupDesc{};
  bindGroupDesc.layout = mBindGroupLayout;
  // There must be as many bindings as declared in the layout!
  bindGroupDesc.entryCount = 1;
  bindGroupDesc.entries = &binding;
  mBindGroup = mDevice.CreateBindGroup(&bindGroupDesc);
}

bool Application::InitializePipeline() {

  std::cout << "Creating shader module..." << std::endl;
  ShaderModule shaderModule = ResourceManager::loadShaderModule(
      RESOURCE_DIR "/shaders/shader.wgsl", mInstance, mDevice);
  std::cout << "Shader module: " << shaderModule.Get() << std::endl;

  // Check for errors
  if (shaderModule == nullptr) {
    return false;
  }

  // Define binding layout
  BindGroupLayoutEntry bindingLayout{};

  // The binding index as used in the @binding attribute in the shader
  bindingLayout.binding = 0;

  // The stage that needs to access this resource
  bindingLayout.visibility = ShaderStage::Vertex | ShaderStage::Fragment;

  bindingLayout.buffer.type = BufferBindingType::Uniform;
  bindingLayout.buffer.minBindingSize = sizeof(MyUniforms);
  bindingLayout.buffer.hasDynamicOffset = true;

  // Create a bind group layout
  BindGroupLayoutDescriptor bindGroupLayoutDesc{};
  bindGroupLayoutDesc.entryCount = 1;
  bindGroupLayoutDesc.entries = &bindingLayout;
  mBindGroupLayout = mDevice.CreateBindGroupLayout(&bindGroupLayoutDesc);

  RenderPipelineDescriptor pipelineDesc{};
  pipelineDesc.label = "My render pipeline";

  // Vertex fetch
  VertexBufferLayout vertexBufferLayout{};

  std::vector<VertexAttribute> vertexAttribs{3};

  // vertex
  vertexAttribs[0].shaderLocation = 0; // @location(0)
  vertexAttribs[0].format = VertexFormat::Float32x3;
  vertexAttribs[0].offset = offsetof(VertexAttributes, position);

  // Normal attribute
  vertexAttribs[1].shaderLocation = 1; // @location(2)
  vertexAttribs[1].format = VertexFormat::Float32x3;
  vertexAttribs[1].offset = offsetof(VertexAttributes, normal);

  // color
  vertexAttribs[2].shaderLocation = 2; // @location(2)
  vertexAttribs[2].format = VertexFormat::Float32x3;
  vertexAttribs[2].offset = offsetof(VertexAttributes, color);

  vertexBufferLayout.attributeCount =
      static_cast<uint32_t>(vertexAttribs.size());
  vertexBufferLayout.attributes = vertexAttribs.data();

  // == Common to attributes from the same buffer ==
  vertexBufferLayout.arrayStride = sizeof(VertexAttributes);
  vertexBufferLayout.stepMode = VertexStepMode::Vertex;

  // Vertex stage: no vertex buffers yet, positions come from the shader itself
  pipelineDesc.vertex.module = shaderModule;
  pipelineDesc.vertex.entryPoint = "vs_main";

  // When describing the render pipeline:
  pipelineDesc.vertex.bufferCount = 1;
  pipelineDesc.vertex.buffers = &vertexBufferLayout;

  // NB: This is optional, it corresponds to the default values

  // Each sequence of 3 vertices is considered as a triangle
  pipelineDesc.primitive.topology = PrimitiveTopology::TriangleList;

  // We'll see later how to specify the order in which vertices should be
  // connected. When not specified, vertices are considered sequentially.
  pipelineDesc.primitive.stripIndexFormat = IndexFormat::Undefined;

  // The face orientation is defined by assuming that when looking
  // from the front of the face, its corner vertices are enumerated
  // in the counter-clockwise (CCW) order.
  pipelineDesc.primitive.frontFace = FrontFace::CCW;

  // But the face orientation does not matter much because we do not
  // cull (i.e. "hide") the faces pointing away from us (which is often
  // used for optimization).
  pipelineDesc.primitive.cullMode = CullMode::None;

  // Fragment stage: writes one color into the surface texture
  ColorTargetState colorTarget{};
  colorTarget.format = mSurfaceFormat;

  BlendState blendState{};
  // TODO: test
  blendState.color.srcFactor = BlendFactor::SrcAlpha;
  blendState.color.dstFactor = BlendFactor::OneMinusSrcAlpha;
  blendState.color.operation = BlendOperation::Add;
  colorTarget.blend = &blendState;

  FragmentState fragmentState{};
  fragmentState.module = shaderModule;
  fragmentState.entryPoint = "fs_main";
  fragmentState.targetCount = 1;
  fragmentState.targets = &colorTarget;
  pipelineDesc.fragment = &fragmentState;

  // Create the pipeline layout
  PipelineLayoutDescriptor layoutDesc{};
  layoutDesc.bindGroupLayoutCount = 1;
  layoutDesc.bindGroupLayouts = &mBindGroupLayout;
  mLayout = mDevice.CreatePipelineLayout(&layoutDesc);

  pipelineDesc.layout = mLayout;

  DepthStencilState depthStencilState{};
  depthStencilState.depthCompare =
      CompareFunction::Less; // blend if depth is less!
  depthStencilState.depthWriteEnabled = OptionalBool::True;
  depthStencilState.format = mDepthTextureFormat;
  pipelineDesc.depthStencil = &depthStencilState;

  // Primitive assembly (TriangleList), no depth buffer and no multisampling:
  // the defaults are exactly what we want

  mPipeline = mDevice.CreateRenderPipeline(&pipelineDesc);
  return mPipeline != nullptr;
}

void Application::Terminate() {
  mSurface.Unconfigure();
  mSurface = nullptr;
  mPipeline = nullptr;
  mQueue = nullptr;
  mDevice = nullptr;
  mInstance = nullptr;
  mPointCount = 0;
  mIndexCount = 0;
  mPointBuffer = nullptr;
  mIndexBuffer = nullptr;
  mUniformBuffer = nullptr;
  mLayout = nullptr;
  mBindGroupLayout = nullptr;
  mBindGroup = nullptr;
  mDepthTextureView = nullptr;

  glfwDestroyWindow(mWindow);
  glfwTerminate();
}

TextureView Application::GetNextSurfaceView() {
  // Get the next surface texture
  SurfaceTexture surfaceTexture{};
  mSurface.GetCurrentTexture(&surfaceTexture);

  if (surfaceTexture.status != SurfaceGetCurrentTextureStatus::SuccessOptimal &&
      surfaceTexture.status !=
          SurfaceGetCurrentTextureStatus::SuccessSuboptimal) {
    return nullptr;
  }

  // Create surface texture view
  TextureViewDescriptor viewDescriptor{};
  viewDescriptor.label = "Surface texture view";
  viewDescriptor.dimension =
      TextureViewDimension::e2D; // not to confuse with 2DArray
  TextureView targetView = surfaceTexture.texture.CreateView(&viewDescriptor);

  return targetView;
}

void Application::MainLoop() {
  glfwPollEvents();
  mInstance.ProcessEvents();

  // Update view matrix
  mUniforms.time = static_cast<float>(glfwGetTime());
  mQueue.WriteBuffer(mUniformBuffer, offsetof(MyUniforms, time),
                     &mUniforms.time, sizeof(MyUniforms::time));

  mat4x4 R1 = rotate(mat4x4(1.0), -mUniforms.time, vec3(0.0, 1.0, 0.0));

  mUniforms.modelMatrix = R1 * mT1 * mS;
  mQueue.WriteBuffer(mUniformBuffer, offsetof(MyUniforms, modelMatrix),
                     &mUniforms.modelMatrix, sizeof(MyUniforms::modelMatrix));

  // Get the next target texture view
  TextureView targetView = GetNextSurfaceView();
  if (!targetView) {
    return; // no surface texture, we skip this frame
  }

  CommandEncoderDescriptor encoderDesc{};
  encoderDesc.label = "My command encoder";
  CommandEncoder encoder = mDevice.CreateCommandEncoder(&encoderDesc);
  RenderPassDescriptor renderPassDesc{};
  RenderPassColorAttachment colorAttachment{};

  colorAttachment.view = targetView;
  colorAttachment.loadOp = LoadOp::Clear;
  colorAttachment.storeOp = StoreOp::Store;
  colorAttachment.clearValue = Color{0.05, 0.05, 0.05, 1.0};

  renderPassDesc.colorAttachmentCount = 1;
  renderPassDesc.colorAttachments = &colorAttachment;

  // We now add a depth/stencil attachment:
  RenderPassDepthStencilAttachment depthStencilAttachment{};
  // Describe depth/stencil attachment

  // The view of the depth texture
  depthStencilAttachment.view = mDepthTextureView;

  // The initial value of the depth buffer, meaning "far"
  depthStencilAttachment.depthClearValue = 1.0f;

  // Operation settings comparable to the color attachment
  depthStencilAttachment.depthLoadOp = LoadOp::Clear;
  depthStencilAttachment.depthStoreOp = StoreOp::Store;

  // we could turn off writing to the depth buffer globally here
  depthStencilAttachment.depthReadOnly = false; // NB: this is the default
  renderPassDesc.depthStencilAttachment = &depthStencilAttachment;

  RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderPassDesc);

  // Select which render pipeline to use
  renderPass.SetPipeline(mPipeline);

  // Set both vertex and index buffers
  renderPass.SetVertexBuffer(0, mPointBuffer, 0, mPointBuffer.GetSize());
  // The second argument must correspond to the choice of uint16_t or uint32_t
  // we've done when creating the index buffer.
  renderPass.SetIndexBuffer(mIndexBuffer, IndexFormat::Uint32, 0,
                            mIndexBuffer.GetSize());

  uint32_t dynamicOffset = 0;

  // Set binding group
  dynamicOffset = 0 * mUniformStride;
  renderPass.SetBindGroup(0, mBindGroup, 1, &dynamicOffset);
  renderPass.DrawIndexed(mIndexCount, 1, 0, 0, 0);

  // // Set binding group with a different uniform offset
  // dynamicOffset = 1 * mUniformStride;
  // renderPass.SetBindGroup(0, mBindGroup, 1, &dynamicOffset);
  // renderPass.DrawIndexed(mIndexCount, 1, 0, 0, 0);

  renderPass.End();
  CommandBufferDescriptor cmdBufferDescriptor{};
  cmdBufferDescriptor.label = "Command buffer";
  CommandBuffer command = encoder.Finish(&cmdBufferDescriptor);

  // Finally submit the command queue
  mQueue.Submit(1, &command);
// At the end of the frame
#ifndef __EMSCRIPTEN__
  mSurface.Present();
#endif
}

bool Application::IsRunning() { return !glfwWindowShouldClose(mWindow); }
