#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int32 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_buffer_reference2 : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_atomic_int64 : enable
#extension GL_KHR_shader_subgroup_vote : require
#extension GL_KHR_shader_subgroup_ballot : require
#extension GL_KHR_shader_subgroup_shuffle : require
#extension GL_KHR_shader_subgroup_basic : require
#extension GL_KHR_shader_subgroup_arithmetic : require

#include "shaderio.h"

layout(scalar, binding = BINDINGS_FRAME_UBO, set = 0) uniform frameConstantsBuffer { FrameConstants view; };
layout(scalar, binding = BINDINGS_READBACK_SSBO, set = 0) buffer readbackBuffer { Readback readback; };
layout(scalar, binding = BINDINGS_RENDERINSTANCES_SSBO, set = 0) buffer renderInstancesBuffer { RenderInstance instances[]; };
layout(scalar, binding = BINDINGS_GEOMETRIES_SSBO, set = 0) buffer geometryBuffer { Geometry geometries[]; };
layout(scalar, binding = BINDINGS_SCENEBUILDING_UBO, set = 0) uniform buildBuffer { SceneBuilding build; };
layout(scalar, binding = BINDINGS_SCENEBUILDING_SSBO, set = 0) coherent buffer buildBufferRW { volatile SceneBuilding buildRW; };

layout(local_size_x = TRAVERSAL_INIT_WORKGROUP) in;

#include "traversal.glsl"

void main()
{
  uint instanceID = getGlobalInvocationIndex(gl_GlobalInvocationID);
  if(instanceID >= build.numRenderInstances)
    return;

  RenderInstance instance = instances[instanceID];
  Geometry geometry = geometries[instance.geometryID];

  uint offsetClusters = atomicAdd(buildRW.renderClusterCounter, 1);
  if(offsetClusters < build.maxRenderClusters)
  {
    ClusterInfo clusterInfo;
    clusterInfo.instanceID = instanceID;
    clusterInfo.clusterID = geometry.lowDetailClusterID;
    build.renderClusterInfos.d[offsetClusters] = clusterInfo;
  }
}
