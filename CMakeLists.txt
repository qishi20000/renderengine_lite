cmake_minimum_required(VERSION 3.18)
project(RenderEngineLite LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(REL_BUILD_SAMPLES "Build desktop preview samples" ON)
option(REL_BUILD_TESTS "Build unit tests" OFF)

# ---------------------------------------------------------------------------
# core library (backend-agnostic: engine/renderer/avm/utils)
# ---------------------------------------------------------------------------
file(GLOB_RECURSE REL_CORE_SOURCES
    CONFIGURE_DEPENDS
    src/engine/*.cpp
    src/renderer/*.cpp
    src/backend/*.cpp        # DriverApi.cpp helpers etc, NOT backend/opengl
    src/avm/*.cpp
    src/utils/*.cpp
)
list(FILTER REL_CORE_SOURCES EXCLUDE REGEX ".*/backend/opengl/.*")
list(FILTER REL_CORE_SOURCES EXCLUDE REGEX ".*/backend/vulkan/.*")

add_library(renderengine_lite STATIC ${REL_CORE_SOURCES})

target_include_directories(renderengine_lite
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ---------------------------------------------------------------------------
# OpenGL ES backend — only implementation shipped in this milestone.
# Requires a real GLES3/EGL toolchain (Android NDK, or ANGLE for desktop
# preview). Not compiled on plain desktop builds without one of those.
# ---------------------------------------------------------------------------
if(ANDROID)
    file(GLOB_RECURSE REL_GLES_SOURCES CONFIGURE_DEPENDS src/backend/opengl/*.cpp)
    target_sources(renderengine_lite PRIVATE
        ${REL_GLES_SOURCES}
        src/platform/android/PlatformEGLAndroid.cpp
    )
    target_link_libraries(renderengine_lite PUBLIC
        EGL
        GLESv3
        android
        log
    )
elseif(REL_GLES_DESKTOP_ANGLE_DIR)
    # Optional: point -DREL_GLES_DESKTOP_ANGLE_DIR=/path/to/angle at a built
    # ANGLE distribution to compile+run the GLES backend on desktop for
    # rapid iteration without an Android device. See samples/desktop_preview.
    file(GLOB_RECURSE REL_GLES_SOURCES CONFIGURE_DEPENDS src/backend/opengl/*.cpp)
    target_sources(renderengine_lite PRIVATE ${REL_GLES_SOURCES})
    target_include_directories(renderengine_lite PUBLIC ${REL_GLES_DESKTOP_ANGLE_DIR}/include)
    target_link_directories(renderengine_lite PUBLIC ${REL_GLES_DESKTOP_ANGLE_DIR}/lib)
    target_link_libraries(renderengine_lite PUBLIC EGL GLESv2)
else()
    message(STATUS "renderengine_lite: no GLES backend compiled (build for ANDROID, "
                    "or pass -DREL_GLES_DESKTOP_ANGLE_DIR=<path-to-angle> for desktop preview)")
endif()

# third_party: glm (header-only) — expected to be a submodule under third_party/glm
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glm)
    target_include_directories(renderengine_lite PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glm)
endif()

if(REL_BUILD_SAMPLES AND NOT ANDROID AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/samples/desktop_preview/CMakeLists.txt)
    add_subdirectory(samples/desktop_preview)
endif()

if(REL_BUILD_TESTS AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt)
    enable_testing()
    add_subdirectory(tests)
endif()
