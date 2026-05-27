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
  src/app/lod_clusters_app.cpp
  src/app/main.cpp
  src/gpu/resources.cpp
  src/lod/cluster_lod_builder.cpp
  src/lod/mesh_lod.cpp
  src/render/cluster_lod_renderer.cpp
  src/render/preloaded_scene.cpp
  src/render/renderer.cpp
  src/scene/cluster_compression.cpp
  src/scene/gltf_loader.cpp
  src/scene/scene.cpp
  src/thirdparty/cgltf.cpp
)
set(HEADER_FILES
  src/app/lod_clusters_app.hpp
  src/gpu/resources.hpp
  src/lod/mesh_lod.h
  src/lod/mesh_lod_bounds.h
  src/lod/mesh_lod_build.h
  src/lod/mesh_lod_clustering.h
  src/lod/mesh_lod_config.h
  src/lod/mesh_lod_impl.h
  src/lod/mesh_lod_local_indices.h
  src/lod/mesh_lod_simplify.h
  src/lod/mesh_lod_types.h
  src/render/preloaded_scene.hpp
  src/render/renderer.hpp
  src/scene/scene.hpp
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
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES} ${SHADER_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/shaders
  ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
)
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
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src PREFIX "Source Files" FILES ${SOURCE_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src PREFIX "Header Files" FILES ${HEADER_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/shaders PREFIX "Shader Files" FILES ${SHADER_FILES})
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
