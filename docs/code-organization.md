# Code Organization

This project is organized by runtime responsibility rather than by file type.
The CMake target collects source and shader files recursively, so new files can be
added inside the existing module folders without changing the top-level build file.

## C++ Modules

- `src/app`: application entry point, UI, and top-level `LodClusters` orchestration.
- `src/core`: processed-scene cache and binary serialization helpers.
- `src/scene`: scene data model, glTF import, geometry storage, instances, materials, bounds, and scene processing.
- `src/meshlod`: standalone mesh LOD generation implementation and supporting headers.
- `src/renderer`: Vulkan resource management, shader compilation, Hi-Z, preloaded scene upload, renderer abstractions, and the cluster LOD render path.
- `src/streaming`: streaming scene upload, resident data, request/update tasks, and allocator utilities.
- `src/vendor`: single-file third-party integration units that are compiled with the target.

## Shader Modules

- `shaders/shared/interface`: shared CPU/GPU layout headers and shader interface definitions.
- `shaders/shared/common`: reusable shader helpers, culling code, shading helpers, and attribute encoding.
- `shaders/passes/cluster`: primary cluster rendering shaders and software-raster cluster pass.
- `shaders/passes/debug`: cluster and instance bounding-box visualization passes.
- `shaders/passes/fullscreen`: full-screen background, resolve, and Hi-Z generation passes.
- `shaders/compute/streaming`: streaming request, setup, and scene update compute shaders.
- `shaders/compute/traversal`: LOD traversal and traversal pre-sort compute shaders.
- `shaders/compute/build`: indirect draw/dispatch setup compute shaders.

## Build Notes

- `CMakeLists.txt` uses `GLOB_RECURSE` with `CONFIGURE_DEPENDS` for `src` and `shaders`.
- C++ include directories are module-level, so includes stay short and independent of file locations.
- Shader compilation paths include the shader module folder, for example `passes/cluster/cluster_mesh.mesh.glsl`.
- Runtime shader include search paths include every shader subdirectory for both source-tree and install-tree layouts.

## Application Split

- `src/app/main.cpp`: process entry point, Vulkan context setup, application elements, command-line parsing, and docking layout.
- `src/app/lodclusters.hpp`: `LodClusters` state, public application callbacks, and private helper declarations grouped by responsibility.
- `src/app/lodclusters_config.cpp`: constructor, command-line parameter registration, and frame default initialization.
- `src/app/lodclusters_scene.cpp`: model/config file loading, cache saving, processing-only mode, render-scene creation, cluster preset mapping, camera placement, and picking helpers.
- `src/app/lodclusters_lifecycle.cpp`: framebuffer resize handling, ImGui viewport texture binding, renderer creation/destruction, attach/detach lifecycle, sequencer memory reports, and screenshot capture.
- `src/app/lodclusters_runtime.cpp`: adaptive software-raster feedback, runtime config reconciliation, camera string application, and per-frame render dispatch.
- `src/app/lodclusters_ui.cpp`: menus, settings windows, statistics windows, streaming memory plots, debug readback UI, and viewport image presentation.

## Implementation Notes

- `CMakeLists.txt` uses recursive source collection, so the application split does not require manual target updates.
- Rendering command order is unchanged by the application split; the per-frame render path still enters through `LodClusters::onRender()` and then `Renderer::render()`.
- Renderer code is split between shared renderer setup in `renderer.cpp`, the cluster LOD implementation in `renderer_clusters_lod.cpp`, framebuffer/resource ownership in `resources.cpp`, and Hi-Z management in `hiz.cpp`.
- Streaming task code is split into request/residency bookkeeping and allocator/update/transfer task execution.
