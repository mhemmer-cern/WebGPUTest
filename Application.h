#pragma once

#include <webgpu/webgpu_cpp.h>

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

  // Get next texture view of the surface
  wgpu::TextureView GetNextSurfaceView();

  // We put here all the variables that are shared between init and main loop
  GLFWwindow* mWindow = nullptr;
  wgpu::Instance mInstance = nullptr;
  wgpu::Device mDevice = nullptr;
  wgpu::Queue mQueue = nullptr;
  wgpu::Surface mSurface = nullptr;
  wgpu::TextureFormat mSurfaceFormat{};
};
