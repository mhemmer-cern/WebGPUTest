# ---------------------------------------------------------------------------
# The app
# ---------------------------------------------------------------------------
add_executable(App main.cpp)

webgputest_set_options(App)

target_link_libraries(App PRIVATE Renderer)

# Put the executable at build/bin/App. Not build/App: on case-insensitive
# filesystems (macOS, Windows) that would clash with the build/app/ folder.
set_target_properties(App PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin
)

if(EMSCRIPTEN)
    target_link_options(App PRIVATE
        "--preload-file" "${PROJECT_SOURCE_DIR}/resources@resources"
        "-sASYNCIFY=1" "-sUSE_GLFW=3")
    set_target_properties(App PROPERTIES SUFFIX ".html")
endif()

if(XCODE)
    set_target_properties(App PROPERTIES
        XCODE_GENERATE_SCHEME ON
        XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal"
    )
endif()
