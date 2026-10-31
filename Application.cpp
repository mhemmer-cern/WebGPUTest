#include "Application.h"
#include "webgpu-utils.h"

#include <GLFW/glfw3.h>

#include <webgpu/webgpu_glfw.h>
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_cpp_print.h>

#include <iostream>
#include <vector>

using namespace wgpu;

bool Application::Initialize(){

  // init GLFW
	if (!glfwInit()) {
    std::cerr << "Could not initialize GLFW!" << std::endl;
    return false;
	}

  // Create the window
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // <-- extra info for glfwCreateWindow
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

	// We create the instance using this descriptor
	#ifdef WEBGPU_BACKEND_EMSCRIPTEN
	mInstance = CreateInstance(nullptr);
	#else //  WEBGPU_BACKEND_EMSCRIPTEN
	mInstance = CreateInstance(&desc);
	#endif //  WEBGPU_BACKEND_EMSCRIPTEN
	// We can check whether there is actually an instance created
	if (!mInstance) {
		std::cerr << "Could not initialize WebGPU!" << std::endl;
		return false;
	}
	
	// Display the object (WGPUInstance is a simple pointer, it may be
	// copied around without worrying about its size).
	std::cout << "WGPU instance: " << mInstance.Get() << std::endl;

  mSurface = glfw::CreateSurfaceForWindow(Instance(mInstance), mWindow);

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
	// We leave 'requiredLimits' untouched for now
	deviceDesc.requiredLimits = &requiredLimits;
	// Make sure that the 'requiredLimits' variable lives until the call to wgpuAdapterRequestDevice!
	deviceDesc.defaultQueue.label = "The Default Queue";
	deviceDesc.SetDeviceLostCallback(
    CallbackMode::AllowSpontaneous,
    [](Device const& device, DeviceLostReason reason, StringView message) {
        std::cout << "Device " << device.Get() << " was lost: reason " << reason
                  << " (" << std::string_view(message) << ")" << std::endl;
    });

  deviceDesc.SetUncapturedErrorCallback(
      [](Device const& device, ErrorType type, StringView message) {
          std::cout << "Uncaptured error in device " << device.Get() << ": type " << type
                    << " (" << std::string_view(message) << ")" << std::endl;
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
  config.format = capabilities.formats[0];  // the first format is the preferred one
  config.presentMode = PresentMode::Fifo;
  config.alphaMode = CompositeAlphaMode::Auto;
  mSurface.Configure(&config);
  mSurfaceFormat = config.format;

  return true;
}

void Application::Terminate(){
  mSurface.Unconfigure();
  mSurface = nullptr;
  mQueue = nullptr;
  mDevice = nullptr;
  mInstance = nullptr;

  glfwDestroyWindow(mWindow);
  glfwTerminate();
}

TextureView Application::GetNextSurfaceView() {
  // Get the next surface texture
  SurfaceTexture surfaceTexture{};
  mSurface.GetCurrentTexture(&surfaceTexture);

  if (surfaceTexture.status != SurfaceGetCurrentTextureStatus::SuccessOptimal &&
      surfaceTexture.status != SurfaceGetCurrentTextureStatus::SuccessSuboptimal) {
    return nullptr;
  }

    // Create surface texture view
    TextureViewDescriptor viewDescriptor{};
    viewDescriptor.label = "Surface texture view";
    viewDescriptor.dimension = TextureViewDimension::e2D; // not to confuse with 2DArray
    TextureView targetView = surfaceTexture.texture.CreateView(&viewDescriptor);
    
    return targetView;
}

void Application::MainLoop() {
	glfwPollEvents();
  mInstance.ProcessEvents();

  // Get the next target texture view
  TextureView targetView = GetNextSurfaceView();
  if (!targetView){
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
	colorAttachment.clearValue = Color{ 1.0, 0.8, 0.55, 1.0 };
	
	renderPassDesc.colorAttachmentCount = 1;
	renderPassDesc.colorAttachments = &colorAttachment;
	
	RenderPassEncoder renderPass = encoder.BeginRenderPass(&renderPassDesc);
	// Use the render pass here (we do nothing with the render pass for now)
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

bool Application::IsRunning() {
  return !glfwWindowShouldClose(mWindow);
}
