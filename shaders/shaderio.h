#ifndef _SHADERIO_H_
#define _SHADERIO_H_

#include "shaderio_core.h"
#include "shaderio_scene.h"
#include "shaderio_building.h"
#include "nvshaders/sky_io.h.slang"

#define VISUALIZE_MATERIAL 0
#define VISUALIZE_GREY 1
#define VISUALIZE_VIS_BUFFER 2
#define VISUALIZE_CLUSTER 3
#define VISUALIZE_GROUP 4
#define VISUALIZE_LOD 5
#define VISUALIZE_TRIANGLE 6
#define VISUALIZE_DEPTH_ONLY 7

#define BINDINGS_FRAME_UBO 0
#define BINDINGS_READBACK_SSBO 1
#define BINDINGS_GEOMETRIES_SSBO 2
#define BINDINGS_RENDERINSTANCES_SSBO 3
#define BINDINGS_SCENEBUILDING_SSBO 4
#define BINDINGS_SCENEBUILDING_UBO 5
#define BINDINGS_RENDER_TARGET 6

#define BUILD_SETUP_TRAVERSAL_RUN 1
#define BUILD_SETUP_DRAW 2

#define TRAVERSAL_INIT_WORKGROUP 64
#define TRAVERSAL_RUN_WORKGROUP 64

#ifdef __cplusplus
namespace shaderio {
using namespace glm;
#else

#ifndef ALLOW_SHADING
#define ALLOW_SHADING 1
#endif

#ifndef ALLOW_VERTEX_NORMALS
#define ALLOW_VERTEX_NORMALS 1
#endif

#ifndef ALLOW_VERTEX_TEXCOORDS
#define ALLOW_VERTEX_TEXCOORDS 1
#endif

#ifndef ALLOW_VERTEX_TANGENTS
#define ALLOW_VERTEX_TANGENTS 1
#endif

#ifndef USE_RENDER_STATS
#define USE_RENDER_STATS 1
#endif

#ifndef USE_MEMORY_STATS
#define USE_MEMORY_STATS 1
#endif

#ifndef USE_TWO_SIDED
#define USE_TWO_SIDED 1
#endif

#ifndef USE_FORCED_TWO_SIDED
#define USE_FORCED_TWO_SIDED 0
#endif

#ifndef MAX_VISIBLE_CLUSTERS
#define MAX_VISIBLE_CLUSTERS 1024
#endif

#ifndef TARGETS_RASTERIZATION
#define TARGETS_RASTERIZATION 1
#endif

#ifndef SUPPORTS_RT
#define SUPPORTS_RT 0
#endif

#endif

struct FrameConstants
{
  mat4 projMatrix;
  mat4 projMatrixI;

  mat4 viewProjMatrix;
  mat4 viewProjMatrixI;
  mat4 viewMatrix;
  mat4 viewMatrixI;
  vec4 viewPos;
  vec4 viewDir;
  vec4 viewPlane;

  mat4 skyProjMatrixI;
  mat4 viewProjMatrixPrev;

  ivec2 viewport;
  vec2  viewportf;

  vec2 viewPixelSize;
  vec2 viewClipSize;

  vec3  wLightPos;
  float lightMixer;

  vec3  wUpDir;
  float sceneSize;

  uint  colorXor;
  uint  visualize;
  float fov;

  float   nearPlane;
  float   farPlane;
  float   ambientOcclusionRadius;
  int32_t ambientOcclusionSamples;

  vec4 nearSizeFactors;

  int   facetShading;
  uint  _pad0;
  vec2  jitter;

  uint  dbgUint;
  float dbgFloat;
  uint  frame;
  uint  doShadow;

  vec4 bgColor;

  uvec2 mousePosition;
  float wireThickness;
  float wireSmoothing;

  vec3 wireColor;
  uint wireStipple;

  vec3  wireBackfaceColor;
  float wireStippleRepeats;

  float wireStippleLength;
  uint  doWireframe;
  uint  visFilterInstanceID;
  uint  visFilterClusterID;

  float time;
  float deltaTime;
  float lodTransitionSpeed;
  float _pad1;

  SkySimpleParameters skyParams;
};

struct Readback
{
  uint     numRenderClusters;
  uint     numTraversalTasks;
  uint     numTraversedTasks;
  uint     numRenderedClusters;
  uint64_t numRenderedTriangles;
  uint64_t numRasteredTriangles;

#ifdef __cplusplus
  uint32_t clusterTriangleId;
  uint32_t _packedDepth0;

  uint32_t instanceId;
  uint32_t _packedDepth1;
#else
  uint64_t clusterTriangleId;
  uint64_t instanceId;
#endif

  uint64_t debugU64;

  int  debugI;
  uint debugUI;
  uint debugF;

  uint debugA[64];
  uint debugB[64];
  uint debugC[64];
};

#ifdef __cplusplus
}
#endif
#endif  // _SHADERIO_H_
