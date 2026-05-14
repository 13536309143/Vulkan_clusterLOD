#version 460

#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int32 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int16 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_buffer_reference2 : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_atomic_int64 : enable
#extension GL_EXT_control_flow_attributes : require
#extension GL_KHR_shader_subgroup_vote : require
#extension GL_KHR_shader_subgroup_ballot : require
#extension GL_KHR_shader_subgroup_shuffle : require
#extension GL_KHR_shader_subgroup_basic : require
#extension GL_KHR_shader_subgroup_clustered : require
#extension GL_KHR_shader_subgroup_arithmetic : require
#extension GL_KHR_memory_scope_semantics : require

#include "shaderio.h"

layout(scalar, binding = BINDINGS_FRAME_UBO, set = 0) uniform frameConstantsBuffer
{
  FrameConstants view;
};

layout(scalar, binding = BINDINGS_READBACK_SSBO, set = 0) buffer readbackBuffer
{
  Readback readback;
};

layout(scalar, binding = BINDINGS_RENDERINSTANCES_SSBO, set = 0) buffer renderInstancesBuffer
{
  RenderInstance instances[];
};

layout(scalar, binding = BINDINGS_GEOMETRIES_SSBO, set = 0) buffer geometryBuffer
{
  Geometry geometries[];
};

#if USE_TWO_PASS_CULLING && TARGETS_RASTERIZATION
layout(binding = BINDINGS_HIZ_TEX) uniform sampler2D texHizFar[2];
#else
layout(binding = BINDINGS_HIZ_TEX) uniform sampler2D texHizFar;
#endif

layout(scalar, binding = BINDINGS_SCENEBUILDING_UBO, set = 0) uniform buildBuffer
{
  SceneBuilding build;
};

layout(scalar, binding = BINDINGS_SCENEBUILDING_SSBO, set = 0) buffer buildBufferRW
{
  SceneBuilding buildRW;
};

#if USE_STREAMING
layout(scalar, binding = BINDINGS_STREAMING_UBO, set = 0) uniform streamingBuffer
{
  SceneStreaming streaming;
};

layout(scalar, binding = BINDINGS_STREAMING_SSBO, set = 0) buffer streamingBufferRW
{
  SceneStreaming streamingRW;
};
#endif

layout(local_size_x = TRAVERSAL_GROUPS_WORKGROUP) in;

#include "culling.glsl"
#include "traversal.glsl"

// Work around older drivers mishandling coherent/volatile buffer references.
#define USE_ATOMIC_LOAD_STORE 1

#if USE_CULLING && (TARGETS_RASTERIZATION || USE_FORCED_INVISIBLE_CULLING)
bool intersectSize(vec4 clipMin, vec4 clipMax, float threshold, float scale)
{
  vec2 rect          = (clipMax.xy - clipMin.xy) * 0.5 * scale * view.viewportf.xy;
  vec2 clipThreshold = vec2(threshold);
  return any(greaterThan(rect, clipThreshold));
}

#if USE_SW_RASTER
struct SwRouteMetrics
{
  uint areaX256;
  uint atomicPressureX256;
  uint depthRiskX256;
  uint scoreX256;
};

vec2 projectedRectPixels(vec4 clipMin, vec4 clipMax)
{
  return max((clipMax.xy - clipMin.xy) * 0.5 * view.viewportf.xy, vec2(0.0));
}

uint metricToX256(float value, float maxValue)
{
  return uint(round(clamp(value, 0.0, maxValue) * 256.0));
}

bool shouldUseSwRaster(BBox bbox, vec4 clipMin, vec4 clipMax, bool clipValid, uint triangleCount, bool twoSided, out SwRouteMetrics metrics)
{
  metrics.areaX256 = 0u;
  metrics.atomicPressureX256 = 0u;
  metrics.depthRiskX256 = 0u;
  metrics.scoreX256 = 0u;

  if(!(clipValid && clipMin.z > 0.0 && clipMax.z < 1.0))
  {
    return false;
  }

#if USE_ADAPTIVE_SW_RASTER_ROUTING
  vec2  rectPixels          = projectedRectPixels(clipMin, clipMax);
  float projectedExtentPx   = max(rectPixels.x, rectPixels.y);
  float projectedAreaPx     = max(rectPixels.x * rectPixels.y, 1.0);
  float triangleCountF      = max(float(triangleCount), 1.0);
  float triangleDensity     = triangleCountF / projectedAreaPx;
  float avgTrianglePixels   = projectedAreaPx / triangleCountF;
  float depthSpan           = max(clipMax.z - clipMin.z, 0.0);

  float extentThreshold     = max(build.swRasterThreshold, 1.0);
  float densityThreshold    = max(build.swRasterTriangleDensityThreshold, 1e-4);
  float scoreThreshold      = clamp(build.swRasterScoreThreshold, 0.05, 0.95);

  float smallClusterScore   = 1.0 - smoothstep(extentThreshold, extentThreshold * 2.0, projectedExtentPx);
  float tinyTriangleScore   = smoothstep(densityThreshold * 0.5, densityThreshold * 2.0, triangleDensity);
  float workScore           = smoothstep(4.0, 32.0, triangleCountF);
  float meshShaderPressure  = smallClusterScore * smoothstep(16.0, 96.0, triangleCountF);

  // Compute raster becomes expensive when large triangles cover many pixels,
  // when many fragments contend for the same atomic target, or when two-sided
  // material work increases the per-triangle path length.
  float atomicConflictRisk  = clamp(avgTrianglePixels / 4.0, 0.0, 1.0) * clamp(triangleDensity / max(densityThreshold, 1e-4), 0.0, 2.0);
  float depthComplexityRisk = clamp(log2(projectedAreaPx + 1.0) / 12.0, 0.0, 1.0) * clamp(depthSpan * 64.0, 0.0, 1.0);
  float materialCostRisk    = twoSided ? 0.20 : 0.0;

  float swBenefitScore = smallClusterScore * 0.35 + tinyTriangleScore * 0.35 + workScore * 0.20 + meshShaderPressure * 0.25;
  float swCostScore    = atomicConflictRisk * 0.35 + depthComplexityRisk * 0.20 + materialCostRisk;
  float routeScore     = swBenefitScore - swCostScore;

  metrics.areaX256           = metricToX256(projectedAreaPx, 65535.0);
  metrics.atomicPressureX256 = metricToX256(atomicConflictRisk * projectedAreaPx, 65535.0);
  metrics.depthRiskX256      = metricToX256(depthComplexityRisk * projectedAreaPx, 65535.0);
  metrics.scoreX256          = metricToX256(max(routeScore, 0.0), 4.0);

  bool withinComputeEnvelope = projectedExtentPx <= extentThreshold * 2.0 && projectedAreaPx <= max(extentThreshold * extentThreshold * 4.0, 1.0);
  bool enoughWork            = triangleCount >= 4u;

  return withinComputeEnvelope && enoughWork && routeScore >= scoreThreshold;
#else
  vec3  bboxDim      = bbox.hi - bbox.lo;
  float bboxDiagonal = max(length(bboxDim), 1e-6);
  float relativeSize = bbox.longestEdge / bboxDiagonal;
  vec2  rectPixels   = projectedRectPixels(clipMin, clipMax);
  float projectedAreaPx = max(rectPixels.x * rectPixels.y, 1.0);
  metrics.areaX256 = metricToX256(projectedAreaPx, 65535.0);

  return !intersectSize(clipMin, clipMax, build.swRasterThreshold, relativeSize);
#endif
}
#endif

bool queryWasVisible(mat4x3 instanceTransform, BBox bbox, out vec4 outClipMin, out vec4 outClipMax, out bool outClipValid)
{
  vec3 bboxMin = bbox.lo;
  vec3 bboxMax = bbox.hi;
  vec4 clipMin;
  vec4 clipMax;
  bool clipValid;
  bool useOcclusion = true;

  bool inFrustum = intersectFrustum(build.cullViewProjMatrixLast, bboxMin, bboxMax, instanceTransform, clipMin, clipMax, clipValid);
  bool isVisible = inFrustum
                   && (!useOcclusion || !clipValid || (intersectSize(clipMin, clipMax, 1.0) && intersectHiz(clipMin, clipMax, 0)));

#if USE_TWO_PASS_CULLING
  if(build.pass == 1)
  {
    if(isVisible)
    {
      isVisible = false;
    }
    else
    {
      inFrustum = intersectFrustum(build.cullViewProjMatrix, bboxMin, bboxMax, instanceTransform, clipMin, clipMax, clipValid);
      isVisible = inFrustum && (!clipValid || (intersectSize(clipMin, clipMax, 1.0) && intersectHiz(clipMin, clipMax, 1)));
    }
  }
#endif

  outClipMin   = clipMin;
  outClipMax   = clipMax;
  outClipValid = clipValid;
  return isVisible;
}
#endif

void main()
{
  uint threadReadIndex = getGlobalInvocationIndex(gl_GlobalInvocationID);
  if(threadReadIndex >= min(build.traversalGroupCounter, build.maxTraversalInfos))
  {
    return;
  }

  TraversalInfo traversalInfo = unpackTraversalInfo(build.traversalGroupInfos.d[threadReadIndex]);
  uint          instanceID    = traversalInfo.instanceID;
  uint          groupIndex    = PACKED_GET(traversalInfo.packedNode, Node_packed_groupIndex);
  uint          groupClusterCount = PACKED_GET(traversalInfo.packedNode, Node_packed_groupClusterCountMinusOne) + 1;

  uint     geometryID = instances[instanceID].geometryID;
  Geometry geometry   = geometries[geometryID];

  mat4x3 worldMatrix     = instances[instanceID].worldMatrix;
  float  uniformScale    = computeUniformScale(worldMatrix);
  float  errorScale      = 1.0;
  mat4x3 traversalMatrix = mat4x3(build.traversalViewMatrix * toMat4(worldMatrix));

#if USE_STREAMING
  Group_in groupRef = Group_in(geometry.streamingGroupAddresses.d[groupIndex]);
  Group    group    = groupRef.d;
  streaming.resident.groups.d[group.residentID].age = uint16_t(0);
#else
  Group_in groupRef = Group_in(geometry.preloadedGroups.d[groupIndex]);
  Group    group    = groupRef.d;
#endif

  for(uint clusterIndex = 0; clusterIndex < groupClusterCount; clusterIndex++)
  {
    bool            forceCluster   = false;
    bool            isValid        = true;
    TraversalMetric traversalMetric;
#if USE_CULLING && (TARGETS_RASTERIZATION || USE_FORCED_INVISIBLE_CULLING)
    BBox bbox = Group_getClusterBBox(groupRef, clusterIndex);
#endif

    uint32_t clusterGeneratingGroup = Group_getGeneratingGroup(groupRef, clusterIndex);
#if USE_STREAMING
    if(clusterGeneratingGroup != SHADERIO_ORIGINAL_MESH_GROUP
       && geometry.streamingGroupAddresses.d[clusterGeneratingGroup] < STREAMING_INVALID_ADDRESS_START)
    {
      traversalMetric = Group_in(geometry.streamingGroupAddresses.d[clusterGeneratingGroup]).d.traversalMetric;
    }
#else
    if(clusterGeneratingGroup != SHADERIO_ORIGINAL_MESH_GROUP)
    {
      traversalMetric = Group_in(geometry.preloadedGroups.d[clusterGeneratingGroup]).d.traversalMetric;
    }
#endif
    else
    {
      traversalMetric = group.traversalMetric;
      forceCluster    = true;
    }

    uint clusterID = group.clusterResidentID + clusterIndex;
    traversalInfo.packedNode = clusterID;

#if USE_CULLING && (TARGETS_RASTERIZATION || USE_FORCED_INVISIBLE_CULLING)
    vec4 clipMin;
    vec4 clipMax;
    bool clipValid;
    isValid = isValid && queryWasVisible(worldMatrix, bbox, clipMin, clipMax, clipValid);
#endif

    bool traverse      = testForTraversal(traversalMatrix, uniformScale, traversalMetric, errorScale);
    bool renderCluster = isValid && (!traverse || forceCluster);

#if TARGETS_RASTERIZATION && USE_SW_RASTER && USE_CULLING
#if USE_STREAMING
    uint triangleCount = Cluster_in(streaming.resident.clusters.d[clusterID]).d.triangleCountMinusOne + 1;
#else
    uint triangleCount = Cluster_in(geometry.preloadedClusters.d[clusterID]).d.triangleCountMinusOne + 1;
#endif
    bool routeCandidate = renderCluster;
    SwRouteMetrics swRouteMetrics;
    bool renderClusterSW = renderCluster && shouldUseSwRaster(bbox, clipMin, clipMax, clipValid, triangleCount,
                                                              instances[instanceID].twoSided != 0, swRouteMetrics);
    if(renderClusterSW)
    {
      renderCluster = false;
    }
#elif TARGETS_RASTERIZATION && USE_SW_RASTER
    bool routeCandidate = false;
    SwRouteMetrics swRouteMetrics;
    swRouteMetrics.areaX256 = 0u;
    swRouteMetrics.atomicPressureX256 = 0u;
    swRouteMetrics.depthRiskX256 = 0u;
    swRouteMetrics.scoreX256 = 0u;
    bool renderClusterSW = false;
#endif

#if TARGETS_RASTERIZATION && USE_SW_RASTER && USE_RENDER_STATS
    uint routeAreaMetric           = routeCandidate ? swRouteMetrics.areaX256 : 0u;
    uint routeAreaMetricSW         = renderClusterSW ? swRouteMetrics.areaX256 : 0u;
    uint routeAtomicMetric         = routeCandidate ? swRouteMetrics.atomicPressureX256 : 0u;
    uint routeAtomicMetricSW       = renderClusterSW ? swRouteMetrics.atomicPressureX256 : 0u;
    uint routeDepthMetric          = routeCandidate ? swRouteMetrics.depthRiskX256 : 0u;
    uint routeDepthMetricSW        = renderClusterSW ? swRouteMetrics.depthRiskX256 : 0u;
    uint routeScoreMetric          = routeCandidate ? swRouteMetrics.scoreX256 : 0u;
    uint routeScoreMetricSW        = renderClusterSW ? swRouteMetrics.scoreX256 : 0u;
    uint sumRouteArea              = subgroupAdd(routeAreaMetric);
    uint sumRouteAreaSW            = subgroupAdd(routeAreaMetricSW);
    uint sumRouteAtomic            = subgroupAdd(routeAtomicMetric);
    uint sumRouteAtomicSW          = subgroupAdd(routeAtomicMetricSW);
    uint sumRouteDepth             = subgroupAdd(routeDepthMetric);
    uint sumRouteDepthSW           = subgroupAdd(routeDepthMetricSW);
    uint sumRouteScore             = subgroupAdd(routeScoreMetric);
    uint sumRouteScoreSW           = subgroupAdd(routeScoreMetricSW);
    if(subgroupElect())
    {
      atomicAdd(readback.routeProjectedAreaX256, uint64_t(sumRouteArea));
      atomicAdd(readback.routeProjectedAreaSWX256, uint64_t(sumRouteAreaSW));
      atomicAdd(readback.routeAtomicPressureX256, uint64_t(sumRouteAtomic));
      atomicAdd(readback.routeAtomicPressureSWX256, uint64_t(sumRouteAtomicSW));
      atomicAdd(readback.routeDepthRiskX256, uint64_t(sumRouteDepth));
      atomicAdd(readback.routeDepthRiskSWX256, uint64_t(sumRouteDepthSW));
      atomicAdd(readback.routeScoreX256, uint64_t(sumRouteScore));
      atomicAdd(readback.routeScoreSWX256, uint64_t(sumRouteScoreSW));
    }
#endif

#if TARGETS_RASTERIZATION && USE_SW_RASTER
    uvec4 voteClustersSW  = subgroupBallot(renderClusterSW);
    uint  countClustersSW = subgroupBallotBitCount(voteClustersSW);
    uint  offsetClustersSW = 0;
#endif

    uvec4 voteClusters  = subgroupBallot(renderCluster);
    uint  countClusters = subgroupBallotBitCount(voteClusters);
    uint  offsetClusters = 0;

    if(subgroupElect())
    {
      offsetClusters = atomicAdd(buildRW.renderClusterCounter, countClusters);
#if TARGETS_RASTERIZATION && USE_SW_RASTER
      offsetClustersSW = atomicAdd(buildRW.renderClusterCounterSW, countClustersSW);
#endif
    }

    offsetClusters = subgroupBroadcastFirst(offsetClusters);
    offsetClusters += subgroupBallotExclusiveBitCount(voteClusters);
    renderCluster = renderCluster && offsetClusters < build.maxRenderClusters;

#if TARGETS_RASTERIZATION && USE_SW_RASTER
    offsetClustersSW = subgroupBroadcastFirst(offsetClustersSW);
    offsetClustersSW += subgroupBallotExclusiveBitCount(voteClustersSW);
    renderClusterSW = renderClusterSW && offsetClustersSW < build.maxRenderClusters;
#endif

#if TARGETS_RASTERIZATION && USE_SW_RASTER
    if(renderCluster || renderClusterSW)
#else
    if(renderCluster)
#endif
    {
#if TARGETS_RASTERIZATION && USE_SW_RASTER
      uint writeIndex          = renderCluster ? offsetClusters : offsetClustersSW;
      uint64s_coh writePointer = uint64s_coh(uint64_t(renderCluster ? build.renderClusterInfos : build.renderClusterInfosSW));
#else
      uint writeIndex          = offsetClusters;
      uint64s_coh writePointer = uint64s_coh(uint64_t(build.renderClusterInfos));
#endif

#if USE_ATOMIC_LOAD_STORE
      atomicStore(writePointer.d[writeIndex], packTraversalInfo(traversalInfo), gl_ScopeDevice, gl_StorageSemanticsBuffer, gl_SemanticsRelease);
#else
      writePointer.d[writeIndex] = packTraversalInfo(traversalInfo);
#endif
      memoryBarrierBuffer();
    }
  }
}
