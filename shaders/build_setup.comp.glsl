#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int32 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_buffer_reference2 : enable
#extension GL_EXT_scalar_block_layout : enable

#include "shaderio.h"

layout(push_constant) uniform pushData { uint setup; } push;

layout(scalar, binding = BINDINGS_READBACK_SSBO, set = 0) buffer readbackBuffer { Readback readback; };
layout(scalar, binding = BINDINGS_SCENEBUILDING_UBO, set = 0) uniform buildBuffer { SceneBuilding build; };
layout(scalar, binding = BINDINGS_SCENEBUILDING_SSBO, set = 0) coherent buffer buildBufferRW { SceneBuilding buildRW; };

layout(local_size_x = 1) in;

void main()
{
  if(push.setup == BUILD_SETUP_TRAVERSAL_RUN)
  {
    int traversalTaskCounter = min(buildRW.traversalTaskCounter, int(build.maxTraversalInfos));
    buildRW.traversalTaskCounter = traversalTaskCounter;
    buildRW.traversalInfoWriteCounter = uint(traversalTaskCounter);
    buildRW.traversalInfoReadCounter = 0;
  }
  else if(push.setup == BUILD_SETUP_DRAW)
  {
    uint renderClusterCounter = buildRW.renderClusterCounter;
    uint numRenderedClusters = min(renderClusterCounter, build.maxRenderClusters);

#if USE_EXT_MESH_SHADER
    uvec3 grid = fit16bitLaunchGrid(numRenderedClusters);
    buildRW.indirectDrawClustersEXT.gridX = grid.x;
    buildRW.indirectDrawClustersEXT.gridY = grid.y;
    buildRW.indirectDrawClustersEXT.gridZ = grid.z;
#else
    buildRW.indirectDrawClustersNV.count = numRenderedClusters;
    buildRW.indirectDrawClustersNV.first = 0;
#endif
    buildRW.numRenderedClusters = numRenderedClusters;
    readback.numRenderClusters = renderClusterCounter;
    readback.numTraversalTasks = buildRW.traversalInfoWriteCounter;

#if USE_RENDER_STATS
    readback.numRenderedClusters = numRenderedClusters;
    readback.numTraversedTasks = buildRW.traversalInfoWriteCounter;
#endif
  }
}
