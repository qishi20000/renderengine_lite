cmake_minimum_required(VERSION 3.22.1)
project(rel_jni_bridge LANGUAGES CXX)

# Pull in the engine itself from the repo root. Keeping renderengine_lite's
# own CMakeLists.txt as the single source of truth (rather than duplicating
# its source list here) means `android/` never drifts out of sync with the
# core library — see ARCHITECTURE.md section 10 for the overall layout.
add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../
    ${CMAKE_CURRENT_BINARY_DIR}/renderengine_lite
)

add_library(rel_jni_bridge SHARED
    jni_bridge.cpp
)

# renderengine_lite only exposes include/ (its backend-agnostic public API)
# to link consumers; src/ is PRIVATE to that target (see root CMakeLists.txt)
# since most of it (engine/, renderer/, backend/) is implementation detail.
# src/avm/ is the one exception: it's the AVM "business layer" meant to be
# driven directly by app/platform code like this JNI bridge (see
# ARCHITECTURE.md section 6), so we add just enough of an include path here
# for "avm/CameraStreamManager.h" etc. to resolve.
target_include_directories(rel_jni_bridge PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../src
)

target_link_libraries(rel_jni_bridge PRIVATE
    renderengine_lite
    android
    log
)
