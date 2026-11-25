#include "ResourceManager.h"

#include <webgpu/webgpu_cpp.h>

#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

using namespace wgpu;

bool ResourceManager::loadGeometry(
  const std::filesystem::path& path,
  std::vector<float>& pointData,
  std::vector<uint16_t>& indexData,
  int dimensions
) {
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
    }
    else if (line == "[indices]") {
      currentSection = Section::Indices;
    }
    else if (line.empty() || line[0] == '#') {
      // Do nothing, this is a comment
    }
    else if (currentSection == Section::Points) {
      std::istringstream iss(line);
      // Get x, y, r, g, b
      for (int i = 0; i < dimensions + 3; ++i) {
        iss >> value;
        pointData.push_back(value);
      }
    }
    else if (currentSection == Section::Indices) {
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

ShaderModule ResourceManager::loadShaderModule(const std::filesystem::path& path, Instance instance, Device device) {
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
    [&](CompilationInfoRequestStatus status, CompilationInfo const* info) {
      if (status != CompilationInfoRequestStatus::Success || !info) return;
        for (size_t i = 0; i < info->messageCount; ++i) {
          const CompilationMessage& msg = info->messages[i];
          const char* kind = msg.type == CompilationMessageType::Error   ? "error"
          : msg.type == CompilationMessageType::Warning ? "warning"
          : "info";
          std::cerr << label << ":" << msg.lineNum << ":" << msg.linePos
          << ": " << kind << ": " << std::string_view(msg.message) << "\n";
          if (msg.type == CompilationMessageType::Error){
            hasErrors = true;
          } 
        }
      });
    instance.WaitAny(f, UINT64_MAX);

  return hasErrors ? nullptr : module;

}
