# WebGPUTest
Small Programm to test WebGPU based on https://developer.chrome.com/docs/web-platform/webgpu/build-app

## Building

```
git clone --recurse-submodules https://github.com/mhemmer-cern/WebGPUTest.git
cd WebGPUTest
cmake -B build
cmake --build build
./build/App
```

On Linux you need the X11 or Wayland development packages; CMake tells you which ones if they're missing.
