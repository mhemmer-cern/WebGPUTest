# WebGPUTest

A small program to test WebGPU. It started from
https://developer.chrome.com/docs/web-platform/webgpu/build-app and uses
[Dawn](https://dawn.googlesource.com/dawn) as the WebGPU implementation and
GLFW for windowing.

## Project structure

```
WebGPUTest/
├── app/                    # The executable
│   ├── CMakeLists.txt
│   └── main.cpp
├── renderer/               # Static library with all rendering code
│   ├── CMakeLists.txt
│   ├── include/Renderer/   # Public headers, included as "Renderer/..."
│   └── src/                # Implementation
├── resources/              # Runtime data, loaded via RESOURCE_DIR
│   ├── models/             # Geometry (.txt) and glTF (.glb) models
│   └── shaders/            # WGSL shaders
├── docs/                   # Doxygen configuration
├── external/
│   └── dawn/               # Dawn, as a git submodule
├── .clang-format           # Code style (LLVM)
└── CMakeLists.txt          # Top-level build: dependencies and settings
```

- **`app`** builds the `App` executable. It contains only `main.cpp` and links
  against `Renderer`.
- **`renderer`** builds the `Renderer` library (`Application`,
  `ResourceManager`, WebGPU helpers). Other code includes its headers by path,
  for example `#include "Renderer/Application.h"`.
- **`resources`** is not copied into the build. Native builds read it directly
  from the source tree, and web builds pack it into the page.
- **Dependencies**: Dawn comes from the submodule. CMake downloads GLM and
  fastgltf automatically through `FetchContent`.

## Initial setup

### Requirements

- CMake 3.24 or newer
- A C++20 compiler (Clang, GCC or MSVC)
- Git and Python 3, which Dawn needs to fetch its own dependencies
- **Linux only:** the X11 or Wayland development packages. CMake tells you
  which ones to install if they are missing.
- Optional: Doxygen for the documentation, and clang-format for formatting

### Getting the code

```sh
git clone https://github.com/mhemmer-cern/WebGPUTest.git
cd WebGPUTest
git submodule update --init
```

Don't use `git clone --recurse-submodules` or `git submodule update --init
--recursive`. Dawn has dozens of submodules of its own (ANGLE, SwiftShader,
Vulkan validation layers, DXC, …), which would add several gigabytes. Instead,
CMake fetches only the dependencies Dawn actually needs during configure
(`DAWN_FETCH_DEPENDENCIES`).

To also skip Dawn's history (several hundred MB), you can fetch only the pinned
commit:

```sh
git submodule update --init --depth 1
```

If you already had a clone from before Dawn moved to `external/dawn`, run
this after pulling:

```sh
git submodule sync
git submodule update --init
```

## Building

```sh
cmake -B build
cmake --build build -j
./build/bin/App
```

The first configure and build take a while, because Dawn and its dependencies
are fetched and compiled. Later builds are incremental.

The default build type is `Debug`. For an optimized build:

```sh
cmake -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release
./build-release/bin/App
```

The executable is placed in `build/bin/`, not `build/`. On case-insensitive
filesystems (macOS, Windows), `build/App` would clash with the `build/app/`
directory.

### Xcode (macOS)

```sh
cmake -B build-xcode -G Xcode
open build-xcode/LearnWebGPU.xcodeproj
```

The generated `App` scheme has Metal GPU frame capture enabled.

### Web (Emscripten)

With the Emscripten SDK active:

```sh
emcmake cmake -B build-web
cmake --build build-web
python3 -m http.server -d build-web/bin
```

Then open http://localhost:8000/App.html in a browser that supports WebGPU.
The `resources/` folder is packed into the page.

## Documentation

If Doxygen is installed, CMake adds a `docs` target:

```sh
cmake --build build --target docs
open build/docs/html/index.html
```

## Code style

The code is formatted with clang-format using the `.clang-format` file in the
repository root:

```sh
clang-format -i app/*.cpp renderer/src/*.cpp renderer/include/Renderer/*.h
```
