cmake_minimum_required(VERSION 3.22 FATAL_ERROR)
get_filename_component(PROJECT_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(${PROJECT_NAME} LANGUAGES C CXX)
option(NVPRO2_ENABLE_nvgl OFF)
option(NVPRO2_ENABLE_nvgpu_monitor OFF)
option(NVPRO2_ENABLE_nvslang OFF)
option(NVPRO2_ENABLE_nvvkgltf OFF)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(NvproCore2 REQUIRED)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 20)
set(SOURCE_FILES
  src/cgltf.cpp
  src/clusterlod.cpp
  src/lodclusters.cpp
  src/main.cpp
  src/meshlod.cpp
  src/preloaded.cpp
  src/renderer.cpp
  src/renderer_clusters_lod.cpp
  src/resources.cpp
  src/scene.cpp
  src/scene_cluster_compression.cpp
  src/scene_gltf.cpp
)
set(SHADER_FILES
  shaders/attribute_encoding.h
  shaders/build_setup.comp.glsl
  shaders/clusters.mesh.glsl
  shaders/frag.glsl
  shaders/fullscreen.vert.glsl
  shaders/fullscreen_background.frag.glsl
  shaders/render_shading.glsl
  shaders/shaderio.h
  shaders/shaderio_building.h
  shaders/shaderio_core.h
  shaders/shaderio_scene.h
  shaders/traversal.glsl
  shaders/traversal_init.comp.glsl
  shaders/traversal_run.comp.glsl
)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty)
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${SHADER_FILES})
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} PRIVATE
  nvpro2::nvapp
  nvpro2::nvgui
  nvpro2::nvutils
  nvpro2::nvvk
  nvpro2::nvvkglsl
  meshoptimizer
  cgltf
)
source_group("Shader Files" FILES ${SHADER_FILES})
source_group("Source Files" FILES ${SOURCE_FILES})
add_project_definitions(${PROJECT_NAME})
copy_to_runtime_and_install( ${PROJECT_NAME}
    FILES ${DLSS_DLLS}
    NVSHADERS_FILES "${NVSHADERS_DIR}/nvshaders/slang_types.h"
                    "${NVSHADERS_DIR}/nvshaders/sky_io.h.slang"
                    "${NVSHADERS_DIR}/nvshaders/sky_functions.h.slang"
                    "${NVSHADERS_DIR}/nvshaders/constants.h.slang"
    LOCAL_DIRS "${CMAKE_CURRENT_LIST_DIR}/shaders"
    AUTO
)
