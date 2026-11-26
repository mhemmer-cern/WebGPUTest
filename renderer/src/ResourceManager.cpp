#include "Renderer/ResourceManager.h"

#include "Renderer/VertexAttribute.h"

#include <webgpu/webgpu_cpp.h>

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace wgpu;

bool ResourceManager::loadGeometry(const std::filesystem::path &path,
                                   std::vector<float> &pointData,
                                   std::vector<uint16_t> &indexData,
                                   int dimensions) {
  std::ifstream file(path);
  if (!file.is_open()) {
    std::cerr << "Could not load geometry!" << std::endl;
    return false;
  }

  pointData.clear();
  indexData.clear();

  enum class Section {
    None,
    Points,
    Indices,
  };
  Section currentSection = Section::None;

  float value;
  uint16_t index;
  std::string line;
  while (!file.eof()) {
    getline(file, line);

    // overcome the `CRLF` problem
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }

    if (line == "[points]") {
      currentSection = Section::Points;
    } else if (line == "[indices]") {
      currentSection = Section::Indices;
    } else if (line.empty() || line[0] == '#') {
      // Do nothing, this is a comment
    } else if (currentSection == Section::Points) {
      std::istringstream iss(line);
      // Get x, y, r, g, b
      for (int i = 0; i < dimensions + 3; ++i) {
        iss >> value;
        pointData.push_back(value);
      }
    } else if (currentSection == Section::Indices) {
      std::istringstream iss(line);
      // Get corners #0 #1 and #2
      for (int i = 0; i < 3; ++i) {
        iss >> index;
        indexData.push_back(index);
      }
    }
  }
  return true;
}

bool ResourceManager::loadGeometryFromGltf(
    const std::filesystem::path &path,
    std::vector<VertexAttributes> &vertexData,
    std::vector<uint32_t> &indexData) {

  // read the file from path into memory
  auto data = fastgltf::GltfDataBuffer::FromPath(path);
  if (data.error() != fastgltf::Error::None) {
    std::cerr << "Could not read " << path << ": "
              << fastgltf::getErrorMessage(data.error()) << std::endl;
    return false;
  }

  // parse the file
  // Detects .glb vs .gltf by itself; also loads external .bin files
  fastgltf::Parser parser;
  auto asset = parser.loadGltf(data.get(), path.parent_path(),
                               fastgltf::Options::LoadExternalBuffers);
  if (asset.error() != fastgltf::Error::None) {
    std::cerr << "Could not load " << path << ": "
              << fastgltf::getErrorMessage(asset.error()) << std::endl;
    return false;
  }

  vertexData.clear();
  indexData.clear();

  // loop over meshes aka objects
  for (const auto &mesh : asset->meshes) {
    // loop over primitives aka position, normals and so on
    for (const auto &primitive : mesh.primitives) {

      // Currently we only draw triangles, no points or lines
      if (primitive.type != fastgltf::PrimitiveType::Triangles) {
        continue;
      }

      auto positionIt = primitive.findAttribute("POSITION");
      if (positionIt == primitive.attributes.end()) {
        continue;
      }
      const auto &positionAccessor =
          asset->accessors[positionIt->accessorIndex];

      // Fallback color: the material's base color, or white
      glm::vec3 baseColor(1.0f);
      if (primitive.materialIndex.has_value()) {
        const auto &factor =
            asset->materials[*primitive.materialIndex].pbrData.baseColorFactor;
        baseColor = glm::vec3(factor[0], factor[1], factor[2]);
      }

      // Fill this primitive's vertices with defaults, then overwrite
      // with whatever attributes the file provides
      const auto base = static_cast<uint32_t>(vertexData.size());
      vertexData.resize(base + positionAccessor.count,
                        VertexAttributes{glm::vec3(0.0f),
                                         glm::vec3(0.0f, 0.0f, 1.0f),
                                         baseColor});

      fastgltf::iterateAccessorWithIndex<glm::vec3>(
          asset.get(), positionAccessor,
          [&](glm::vec3 position, std::size_t i) -> void {
            vertexData[base + i].position = position;
          });

      if (auto it = primitive.findAttribute("NORMAL");
          it != primitive.attributes.end()) {
        fastgltf::iterateAccessorWithIndex<glm::vec3>(
            asset.get(), asset->accessors[it->accessorIndex],
            [&](glm::vec3 normal, std::size_t i) -> void {
              vertexData[base + i].normal = normal;
            });
      }

      // Vertex colors may be stored as vec3 or vec4 (RGBA); we keep RGB
      if (auto it = primitive.findAttribute("COLOR_0");
          it != primitive.attributes.end()) {
        const auto &colorAccessor = asset->accessors[it->accessorIndex];
        if (colorAccessor.type == fastgltf::AccessorType::Vec4) {
          fastgltf::iterateAccessorWithIndex<glm::vec4>(
              asset.get(), colorAccessor, [&](glm::vec4 color, std::size_t i) {
                vertexData[base + i].color = glm::vec3(color);
              });
        } else {
          fastgltf::iterateAccessorWithIndex<glm::vec3>(
              asset.get(), colorAccessor, [&](glm::vec3 color, std::size_t i) {
                vertexData[base + i].color = color;
              });
        }
      }

      // Each primitive's indices start at 0, so shift them by 'base'
      if (primitive.indicesAccessor.has_value()) {
        fastgltf::iterateAccessor<uint32_t>(
            asset.get(), asset->accessors[*primitive.indicesAccessor],
            [&](uint32_t index) { indexData.push_back(base + index); });
      } else {
        for (uint32_t i = 0; i < positionAccessor.count; ++i) {
          indexData.push_back(base + i);
        }
      }
    }
  }

  if (vertexData.empty()) {
    std::cerr << "No triangle geometry found in " << path << std::endl;
    return false;
  }
  return true;
}

ShaderModule
ResourceManager::loadShaderModule(const std::filesystem::path &path,
                                  Instance instance, Device device) {
  std::ifstream file(path);
  if (!file.is_open()) {
    std::cerr << "Could not load shader!" << std::endl;
    return nullptr;
  }
  file.seekg(0, std::ios::end);
  size_t size = file.tellg();
  std::string shaderSource(size, ' ');
  file.seekg(0);
  file.read(shaderSource.data(), size);

  ShaderSourceWGSL wgslDesc{};
  wgslDesc.code = StringView(shaderSource);
  ShaderModuleDescriptor shaderDesc{};
  shaderDesc.nextInChain = &wgslDesc;
  std::string label = path.filename().string();
  shaderDesc.label = std::string_view(label);
  ShaderModule module = device.CreateShaderModule(&shaderDesc);
  bool hasErrors = false;
  Future f = module.GetCompilationInfo(
      CallbackMode::WaitAnyOnly,
      [&](CompilationInfoRequestStatus status, CompilationInfo const *info) {
        if (status != CompilationInfoRequestStatus::Success || !info)
          return;
        for (size_t i = 0; i < info->messageCount; ++i) {
          const CompilationMessage &msg = info->messages[i];
          const char *kind = msg.type == CompilationMessageType::Error ? "error"
                             : msg.type == CompilationMessageType::Warning
                                 ? "warning"
                                 : "info";
          std::cerr << label << ":" << msg.lineNum << ":" << msg.linePos << ": "
                    << kind << ": " << std::string_view(msg.message) << "\n";
          if (msg.type == CompilationMessageType::Error) {
            hasErrors = true;
          }
        }
      });
  instance.WaitAny(f, UINT64_MAX);

  return hasErrors ? nullptr : module;
}
