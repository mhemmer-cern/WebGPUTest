cmake_minimum_required(VERSION 3.24)

project(
    LearnWebGPU
    VERSION 0.1.0
    LANGUAGES CXX C
)

# Default to Debug when no build type is given (Dawn needs one)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(App
    main.cpp
    webgpu-utils.h
    webgpu-utils.cpp
)

set_target_properties(App PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    COMPILE_WARNING_AS_ERROR ON
)

if(MSVC)
    target_compile_options(App PRIVATE /W4)
else()
    target_compile_options(App PRIVATE -Wall -Wextra -pedantic)
endif()

set(DAWN_FETCH_DEPENDENCIES ON)
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/dawn/CMakeLists.txt")
    message(FATAL_ERROR
        "Dawn submodule is missing. Run: git submodule update --init")
endif()
add_subdirectory("dawn" EXCLUDE_FROM_ALL)

if(EMSCRIPTEN)
    # Web build: WebGPU and GLFW come from the browser via Emscripten
    target_link_libraries(App PRIVATE emdawnwebgpu_cpp webgpu_glfw)
    set_target_properties(App PROPERTIES SUFFIX ".html")
    target_link_options(App PRIVATE "-sASYNCIFY=1" "-sUSE_GLFW=3")
else()
    # Native build: link Dawn itself
    target_link_libraries(App PRIVATE webgpu_dawn webgpu_glfw glfw)
endif()

if(XCODE)
    set_target_properties(App PROPERTIES
        XCODE_GENERATE_SCHEME ON
        XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal"
    )
endif()
