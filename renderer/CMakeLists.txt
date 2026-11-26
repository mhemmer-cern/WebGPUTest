# ---------------------------------------------------------------------------
# Renderer library
# ---------------------------------------------------------------------------
add_library(Renderer STATIC
    src/Application.cpp
    src/ResourceManager.cpp
    src/webgpu-utils.cpp
    include/Renderer/Application.h
    include/Renderer/ResourceManager.h
    include/Renderer/webgpu-utils.h
)

webgputest_set_options(Renderer)

target_include_directories(Renderer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# PUBLIC: the headers include GLM, so the library and every user must agree
target_compile_definitions(Renderer PUBLIC
    GLM_FORCE_DEPTH_ZERO_TO_ONE
)

if(EMSCRIPTEN)
    # Web: the app packs resources/ into the page, available as "resources/..."
    target_compile_definitions(Renderer PRIVATE RESOURCE_DIR="resources")
    target_link_libraries(Renderer
        PUBLIC  emdawnwebgpu_cpp webgpu_glfw glm::glm
        PRIVATE fastgltf::fastgltf)
else()
    # Native: read directly from the source folder
    target_compile_definitions(Renderer PRIVATE
        RESOURCE_DIR="${PROJECT_SOURCE_DIR}/resources")
    target_link_libraries(Renderer
        PUBLIC  webgpu_dawn webgpu_glfw glfw glm::glm
        PRIVATE fastgltf::fastgltf)
endif()
