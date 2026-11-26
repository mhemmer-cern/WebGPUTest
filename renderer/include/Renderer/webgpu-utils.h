#pragma once

#include <webgpu/webgpu_cpp.h>

#include <functional>

/**
 * Request an adapter and block until it is available.
 * The instance must have been created with the TimedWaitAny feature.
 * Returns a null adapter on failure (test with `if (!adapter)`).
 */
wgpu::Adapter requestAdapterSync(wgpu::Instance const &instance,
                                 wgpu::RequestAdapterOptions const &options);

/**
 * Request a device and block until it is available.
 * Returns a null device on failure (test with `if (!device)`).
 */
wgpu::Device requestDeviceSync(wgpu::Instance const &instance,
                               wgpu::Adapter const &adapter,
                               wgpu::DeviceDescriptor const &descriptor);

/** Print the adapter's limits, features and properties. */
void inspectAdapter(wgpu::Adapter const &adapter);

/** Print the device's features and limits. */
void inspectDevice(wgpu::Device const &device);

uint32_t ceilToNextMultiple(uint32_t value, uint32_t step);

/**
 * Map a buffer for reading, wait until it is mapped, call `processBufferData`
 * with a pointer to its contents, then unmap it again.
 * The buffer must have been created with wgpu::BufferUsage::MapRead.
 * Returns false if the mapping failed.
 */
bool fetchBufferDataSync(
    wgpu::Instance const &instance, wgpu::Buffer const &buffer,
    std::function<void(void const *)> const &processBufferData);
