# Code Organization

This project is organized by runtime responsibility rather than by file type.
The CMake target collects source and shader files recursively, so new files can be
added inside the existing module folders without changing the top-level build file.

## C++ Modules

- `src/app`: application entry point, UI, and top-level `LodClusters` orchestration.
- `src/core`: shared infrastructure that is not tied to one rendering subsystem, such as cache and serialization helpers.
- `src/scene`: scene data, glTF loading, cluster compression, and LOD cluster construction.
- `src/renderer`: Vulkan resources, Hi-Z, preloaded scene upload, render scene abstraction, and cluster LOD rendering.
- `src/streaming`: streaming scene upload, resident data, request/update tasks, and allocator utilities.
- `src/meshlod`: mesh LOD generation implementation and supporting headers.
- `src/vendor`: single-file third-party integration units that are compiled with the target.

## Shader Modules

- `shaders/interface`: shared CPU/GPU layout headers and shader interface definitions.
- `shaders/common`: reusable shader helpers, culling code, and attribute encoding.
- `shaders/render`: primary cluster rendering and software-raster compute shaders.
- `shaders/debug`: cluster and instance bounding-box visualization shaders.
- `shaders/post`: full-screen passes and Hi-Z generation.
- `shaders/streaming`: streaming request, setup, and scene update compute shaders.
- `shaders/traversal`: LOD traversal and traversal pre-sort compute shaders.
- `shaders/build`: indirect draw/dispatch setup shaders.

## Build Notes

- `CMakeLists.txt` uses `GLOB_RECURSE` with `CONFIGURE_DEPENDS` for `src` and `shaders`.
- C++ include directories are module-level, so includes stay short and independent of file locations.
- Shader compilation paths include the shader module folder, for example `render/clusters.mesh.glsl`.
- Runtime shader include search paths include every shader subdirectory for both source-tree and install-tree layouts.
