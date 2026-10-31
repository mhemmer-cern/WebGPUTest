#include "webgpu-utils.h"

#include <webgpu/webgpu_cpp_print.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

using namespace wgpu;

Adapter requestAdapterSync(Instance const& instance, RequestAdapterOptions const& options) {
	Adapter adapter;

	// The lambda may capture variables directly; the C++ wrapper takes care
	// of the userdata pointer that the C API needed.
	Future future = instance.RequestAdapter(
		&options,
		CallbackMode::WaitAnyOnly,
		[&adapter](RequestAdapterStatus status, Adapter result, StringView message) {
			if (status != RequestAdapterStatus::Success) {
				std::cerr << "Error while requesting adapter: " << std::string_view(message) << std::endl;
				return;
			}
			adapter = std::move(result);
		});

	// Block until this request has finished (no timeout)
	instance.WaitAny(future, UINT64_MAX);
	return adapter;
}

Device requestDeviceSync( Instance const& instance, Adapter const& adapter, DeviceDescriptor const& descriptor) {
	Device device;

	Future future = adapter.RequestDevice(
		&descriptor,
		CallbackMode::WaitAnyOnly,
		[&device](RequestDeviceStatus status, Device result, StringView message) {
			if (status != RequestDeviceStatus::Success) {
				std::cerr << "Error while requesting device: " << std::string_view(message) << std::endl;
				return;
			}
			device = std::move(result);
		});

	instance.WaitAny(future, UINT64_MAX);
	return device;
}

void inspectAdapter(Adapter const& adapter) {
	Limits limits{};
	if (adapter.GetLimits(&limits) == Status::Success) {
		std::cout << "Adapter limits:" << std::endl;
		std::cout << " - maxTextureDimension1D: " << limits.maxTextureDimension1D << std::endl;
		std::cout << " - maxTextureDimension2D: " << limits.maxTextureDimension2D << std::endl;
		std::cout << " - maxTextureDimension3D: " << limits.maxTextureDimension3D << std::endl;
		std::cout << " - maxTextureArrayLayers: " << limits.maxTextureArrayLayers << std::endl;
	}

	// SupportedFeatures frees its memory by itself when it goes out of scope
	SupportedFeatures features;
	adapter.GetFeatures(&features);
	std::cout << "Adapter features:" << std::endl;
	for (size_t i = 0; i < features.featureCount; ++i) {
		std::cout << " - " << features.features[i] << std::endl;
	}

	// AdapterInfo also frees its strings by itself
	AdapterInfo info;
	if (adapter.GetInfo(&info) == Status::Success) {
		std::cout << "Adapter properties:" << std::endl;
		std::cout << " - vendorID: " << info.vendorID << std::endl;
		std::cout << " - vendorName: " << std::string_view(info.vendor) << std::endl;
		std::cout << " - architecture: " << std::string_view(info.architecture) << std::endl;
		std::cout << " - deviceID: " << info.deviceID << std::endl;
		std::cout << " - name: " << std::string_view(info.device) << std::endl;
		std::cout << " - driverDescription: " << std::string_view(info.description) << std::endl;
		std::cout << " - adapterType: " << info.adapterType << std::endl;
		std::cout << " - backendType: " << info.backendType << std::endl;
	}
}

void inspectDevice(Device const& device) {
	SupportedFeatures features;
	device.GetFeatures(&features);
	std::cout << "Device features:" << std::endl;
	for (size_t i = 0; i < features.featureCount; ++i) {
		std::cout << " - " << features.features[i] << std::endl;
	}

	Limits limits{};
	if (device.GetLimits(&limits) == Status::Success) {
		std::cout << "Device limits:" << std::endl;
		std::cout << " - maxTextureDimension1D: " << limits.maxTextureDimension1D << std::endl;
		std::cout << " - maxTextureDimension2D: " << limits.maxTextureDimension2D << std::endl;
		std::cout << " - maxTextureDimension3D: " << limits.maxTextureDimension3D << std::endl;
		std::cout << " - maxTextureArrayLayers: " << limits.maxTextureArrayLayers << std::endl;
		std::cout << " - maxBindGroups: " << limits.maxBindGroups << std::endl;
		std::cout << " - maxBindGroupsPlusVertexBuffers: " << limits.maxBindGroupsPlusVertexBuffers << std::endl;
		std::cout << " - maxBindingsPerBindGroup: " << limits.maxBindingsPerBindGroup << std::endl;
		std::cout << " - maxDynamicUniformBuffersPerPipelineLayout: " << limits.maxDynamicUniformBuffersPerPipelineLayout << std::endl;
		std::cout << " - maxDynamicStorageBuffersPerPipelineLayout: " << limits.maxDynamicStorageBuffersPerPipelineLayout << std::endl;
		std::cout << " - maxSampledTexturesPerShaderStage: " << limits.maxSampledTexturesPerShaderStage << std::endl;
		std::cout << " - maxSamplersPerShaderStage: " << limits.maxSamplersPerShaderStage << std::endl;
		std::cout << " - maxStorageBuffersPerShaderStage: " << limits.maxStorageBuffersPerShaderStage << std::endl;
		std::cout << " - maxStorageTexturesPerShaderStage: " << limits.maxStorageTexturesPerShaderStage << std::endl;
		std::cout << " - maxUniformBuffersPerShaderStage: " << limits.maxUniformBuffersPerShaderStage << std::endl;
		std::cout << " - maxUniformBufferBindingSize: " << limits.maxUniformBufferBindingSize << std::endl;
		std::cout << " - maxStorageBufferBindingSize: " << limits.maxStorageBufferBindingSize << std::endl;
		std::cout << " - minUniformBufferOffsetAlignment: " << limits.minUniformBufferOffsetAlignment << std::endl;
		std::cout << " - minStorageBufferOffsetAlignment: " << limits.minStorageBufferOffsetAlignment << std::endl;
		std::cout << " - maxVertexBuffers: " << limits.maxVertexBuffers << std::endl;
		std::cout << " - maxBufferSize: " << limits.maxBufferSize << std::endl;
		std::cout << " - maxVertexAttributes: " << limits.maxVertexAttributes << std::endl;
		std::cout << " - maxVertexBufferArrayStride: " << limits.maxVertexBufferArrayStride << std::endl;
		std::cout << " - maxInterStageShaderVariables: " << limits.maxInterStageShaderVariables << std::endl;
		std::cout << " - maxColorAttachments: " << limits.maxColorAttachments << std::endl;
		std::cout << " - maxColorAttachmentBytesPerSample: " << limits.maxColorAttachmentBytesPerSample << std::endl;
		std::cout << " - maxComputeWorkgroupStorageSize: " << limits.maxComputeWorkgroupStorageSize << std::endl;
		std::cout << " - maxComputeInvocationsPerWorkgroup: " << limits.maxComputeInvocationsPerWorkgroup << std::endl;
		std::cout << " - maxComputeWorkgroupSizeX: " << limits.maxComputeWorkgroupSizeX << std::endl;
		std::cout << " - maxComputeWorkgroupSizeY: " << limits.maxComputeWorkgroupSizeY << std::endl;
		std::cout << " - maxComputeWorkgroupSizeZ: " << limits.maxComputeWorkgroupSizeZ << std::endl;
		std::cout << " - maxComputeWorkgroupsPerDimension: " << limits.maxComputeWorkgroupsPerDimension << std::endl;
	}
}

ShaderModule loadShaderModule(
	Device const& device,
	std::filesystem::path const& path
) {
	std::ifstream file(path);
	if (!file) {
		std::cerr << "Could not open shader file: " << path << std::endl;
		return nullptr;
	}
	std::stringstream buffer;
	buffer << file.rdbuf();

	// Both strings must stay alive until CreateShaderModule() returns,
	// because StringView only points to their characters.
	std::string code = buffer.str();
	std::string label = path.filename().string();

	ShaderSourceWGSL wgsl{};
	wgsl.code = std::string_view(code);

	ShaderModuleDescriptor desc{};
	desc.nextInChain = &wgsl;
	desc.label = std::string_view(label);

	return device.CreateShaderModule(&desc);
}

bool fetchBufferDataSync(
	Instance const& instance,
	Buffer const& buffer,
	std::function<void(void const*)> const& processBufferData
) {
	bool success = false;

	Future future = buffer.MapAsync(
		MapMode::Read,
		0, // offset
		kWholeMapSize,
		CallbackMode::WaitAnyOnly,
		[&success](MapAsyncStatus status, StringView message) {
			if (status == MapAsyncStatus::Success) {
				success = true;
			} else {
				std::cerr << "Could not map buffer: " << status
					<< " (" << std::string_view(message) << ")" << std::endl;
			}
		});

	instance.WaitAny(future, UINT64_MAX);

	if (success) {
		processBufferData(buffer.GetConstMappedRange(0, kWholeMapSize));
		// Unmap so the GPU can use the buffer again
		buffer.Unmap();
	}
	return success;
}
