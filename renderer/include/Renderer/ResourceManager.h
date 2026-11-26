// ResourceManager.h
#pragma once

#include "Renderer/VertexAttribute.h"

#include <webgpu/webgpu_cpp.h>

#include <filesystem>
#include <vector>

class ResourceManager {
public:
  /**
   * Load a file from `path` using our ad-hoc format and populate the
   * `pointData` and `indexData` vectors.
   */
  static bool loadGeometry(const std::filesystem::path &path,
                           std::vector<float> &pointData,
                           std::vector<uint16_t> &indexData,
                           int dimensions = 2);

  /**
   * Create a shader module for a given WebGPU `device` from a WGSL shader
   * source loaded from file `path`.
   */
  static bool loadGeometryFromGltf(const std::filesystem::path &path,
                                   std::vector<VertexAttributes> &vertexData,
                                   std::vector<uint32_t> &indexData);

  /**
   * Create a shader module for a given WebGPU `device` from a WGSL shader
   * source loaded from file `path`.
   */
  static wgpu::ShaderModule loadShaderModule(const std::filesystem::path &path,
                                             wgpu::Instance instance,
                                             wgpu::Device device);

private:
  // [...] Private ResourceManager members
};
