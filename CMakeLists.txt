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

# ---------------------------------------------------------------------------
# Linux: detect which windowing systems can be built (X11 and/or Wayland)
# and configure GLFW and Dawn accordingly. macOS uses Cocoa, nothing to do.
# ---------------------------------------------------------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT EMSCRIPTEN)
    # X11: GLFW needs these extension libraries' headers
    find_package(X11 QUIET)
    if(X11_FOUND AND X11_Xrandr_FOUND AND X11_Xinerama_FOUND AND
       X11_Xcursor_FOUND AND X11_Xi_FOUND AND X11_Xkb_FOUND AND X11_Xshape_FOUND)
        set(HAVE_X11 ON)
    else()
        set(HAVE_X11 OFF)
    endif()

    # Wayland: GLFW needs the wayland-scanner tool and these libraries
    find_package(PkgConfig QUIET)
    find_program(WAYLAND_SCANNER wayland-scanner)
    if(PKG_CONFIG_FOUND AND WAYLAND_SCANNER)
        pkg_check_modules(WAYLAND_LIBS QUIET
            wayland-client wayland-cursor wayland-egl xkbcommon)
    endif()
    if(WAYLAND_SCANNER AND WAYLAND_LIBS_FOUND)
        set(HAVE_WAYLAND ON)
    else()
        set(HAVE_WAYLAND OFF)
    endif()

    if(NOT HAVE_X11 AND NOT HAVE_WAYLAND)
        message(FATAL_ERROR
            "Neither X11 nor Wayland development files were found.\n"
            "Install one of them, e.g.\n"
            "  Debian/Ubuntu X11:     sudo apt install libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxext-dev\n"
            "  Debian/Ubuntu Wayland: sudo apt install libwayland-dev libxkbcommon-dev wayland-protocols\n"
            "  Fedora/RHEL X11:       sudo dnf install libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel libXext-devel\n"
            "  Fedora/RHEL Wayland:   sudo dnf install wayland-devel libxkbcommon-devel wayland-protocols-devel")
    endif()

    message(STATUS "Windowing support: X11=${HAVE_X11}, Wayland=${HAVE_WAYLAND}")

    # GLFW: only build the backends that can actually be compiled here
    set(GLFW_BUILD_X11     ${HAVE_X11}     CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WAYLAND ${HAVE_WAYLAND} CACHE BOOL "" FORCE)
    # Dawn: matching surface support for Vulkan
    set(DAWN_USE_X11       ${HAVE_X11}     CACHE BOOL "" FORCE)
    set(DAWN_USE_WAYLAND   ${HAVE_WAYLAND} CACHE BOOL "" FORCE)
endif()

# ---------------------------------------------------------------------------
# Dawn
# ---------------------------------------------------------------------------
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/dawn/CMakeLists.txt")
    message(FATAL_ERROR "Dawn submodule is missing. Run: git submodule update --init")
endif()

set(DAWN_FETCH_DEPENDENCIES ON)
add_subdirectory("external/dawn" EXCLUDE_FROM_ALL)

# ---------------------------------------------------------------------------
# GLM
# ---------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
	glm
	URL https://github.com/g-truc/glm/archive/refs/tags/1.0.1.tar.gz
    SYSTEM
)

FetchContent_MakeAvailable(glm)

# ---------------------------------------------------------------------------
# fastgltf for loading glTF objects
# ---------------------------------------------------------------------------
FetchContent_Declare(
    fastgltf
    GIT_REPOSITORY https://github.com/spnda/fastgltf.git
    GIT_TAG        v1.0.1
    SYSTEM
)
FetchContent_MakeAvailable(fastgltf)

# ---------------------------------------------------------------------------
# Common settings for our own targets (used by the subdirectories)
# ---------------------------------------------------------------------------
function(webgputest_set_options target)
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        COMPILE_WARNING_AS_ERROR ON
    )
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -pedantic)
    endif()
endfunction()

# ---------------------------------------------------------------------------
# Our code
# ---------------------------------------------------------------------------
add_subdirectory(renderer)
add_subdirectory(app)

# ---------------------------------------------------------------------------
# Documentation (optional): cmake --build build --target docs
# ---------------------------------------------------------------------------
if(NOT EMSCRIPTEN)
    find_package(Doxygen QUIET)
    if(DOXYGEN_FOUND)
        configure_file(docs/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
        add_custom_target(docs
            COMMAND Doxygen::doxygen ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM)
    else()
        message(STATUS "Doxygen not found, 'docs' target unavailable")
    endif()
endif()
