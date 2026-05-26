#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int32 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_buffer_reference2 : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_atomic_int64 : enable
#extension GL_KHR_memory_scope_semantics : require

#include "shaderio.h"

layout(scalar, binding = BINDINGS_FRAME_UBO, set = 0) uniform frameConstantsBuffer { FrameConstants view; };
layout(scalar, binding = BINDINGS_READBACK_SSBO, set = 0) buffer readbackBuffer { Readback readback; };
layout(scalar, binding = BINDINGS_RENDERINSTANCES_SSBO, set = 0) buffer renderInstancesBuffer { RenderInstance instances[]; };
layout(scalar, binding = BINDINGS_GEOMETRIES_SSBO, set = 0) buffer geometryBuffer { Geometry geometries[]; };
layout(scalar, binding = BINDINGS_SCENEBUILDING_UBO, set = 0) uniform buildBuffer { SceneBuilding build; };
layout(scalar, binding = BINDINGS_SCENEBUILDING_SSBO, set = 0) coherent buffer buildBufferRW { volatile SceneBuilding buildRW; };

layout(local_size_x = TRAVERSAL_RUN_WORKGROUP) in;

#include "traversal.glsl"

bool waitForTask(uint readIndex, out TraversalInfo traversalInfo)
{
  uint64_t invalidTask = packUint2x32(uvec2(~0u, ~0u));

  for(uint spin = 0; spin < 1024; spin++)
  {
    uint64_t rawValue = build.traversalNodeInfos.d[readIndex];
    if(rawValue != invalidTask)
    {
      traversalInfo = unpackTraversalInfo(rawValue);
      return traversalInfo.instanceID != ~0u && traversalInfo.packedNode != ~0u;
    }
    memoryBarrierBuffer();
  }

  traversalInfo.instanceID = ~0u;
  traversalInfo.packedNode = ~0u;
  return false;
}

void enqueueNode(TraversalInfo traversalInfo)
{
  uint writeIndex = atomicAdd(buildRW.traversalInfoWriteCounter, 1);
  if(writeIndex < build.maxTraversalInfos)
  {
    build.traversalNodeInfos.d[writeIndex] = packTraversalInfo(traversalInfo);
    memoryBarrierBuffer();
    atomicAdd(buildRW.traversalTaskCounter, 1);
  }
}

void enqueueCluster(uint instanceID, uint clusterID)
{
  uint writeIndex = atomicAdd(buildRW.renderClusterCounter, 1);
  if(writeIndex < build.maxRenderClusters)
  {
    ClusterInfo clusterInfo;
    clusterInfo.instanceID = instanceID;
    clusterInfo.clusterID = clusterID;
    build.renderClusterInfos.d[writeIndex] = clusterInfo;
  }
}

void processTask(TraversalInfo task)
{
  uint instanceID = task.instanceID;
  RenderInstance instance = instances[instanceID];
  Geometry geometry = geometries[instance.geometryID];

  bool isNode = PACKED_GET(task.packedNode, Node_packed_isGroup) == 0;
  uint childCount = 0;
  if(isNode)
    childCount = PACKED_GET(task.packedNode, Node_packed_nodeChildCountMinusOne) + 1;
  else
    childCount = PACKED_GET(task.packedNode, Node_packed_groupClusterCountMinusOne) + 1;

  mat4x3 worldMatrix = instance.worldMatrix;
  float uniformScale = computeUniformScale(worldMatrix);
  mat4x3 instanceToEye = mat4x3(build.traversalViewMatrix * toMat4(worldMatrix));

  for(uint child = 0; child < childCount; child++)
  {
    TraversalInfo nextTask;
    nextTask.instanceID = instanceID;

    TraversalMetric metric;
    bool childIsNode = false;
    bool forceCluster = false;
    uint clusterID = 0;

    if(isNode)
    {
      uint childNodeIndex = PACKED_GET(task.packedNode, Node_packed_nodeChildOffset) + child;
      Node node = geometry.nodes.d[childNodeIndex];
      metric = node.traversalMetric;
      nextTask.packedNode = node.packed;
      childIsNode = PACKED_GET(node.packed, Node_packed_isGroup) == 0;
    }
    else
    {
      uint groupIndex = PACKED_GET(task.packedNode, Node_packed_groupIndex);
      Group_in groupRef = Group_in(geometry.preloadedGroups.d[groupIndex]);
      Group group = groupRef.d;
      uint generatingGroup = Group_getGeneratingGroup(groupRef, child);
      clusterID = group.clusterResidentID + child;

      if(generatingGroup != SHADERIO_ORIGINAL_MESH_GROUP)
        metric = Group_in(geometry.preloadedGroups.d[generatingGroup]).d.traversalMetric;
      else
      {
        metric = group.traversalMetric;
        forceCluster = true;
      }
    }

    bool needsMoreDetail = testForTraversal(instanceToEye, uniformScale, metric, 1.0);
    if(childIsNode && needsMoreDetail)
      enqueueNode(nextTask);
    else if(!childIsNode && (!needsMoreDetail || forceCluster))
      enqueueCluster(instanceID, clusterID);
  }
}

void main()
{
  for(;;)
  {
    uint readIndex = atomicAdd(buildRW.traversalInfoReadCounter, 1);

    for(;;)
    {
      uint writeCount = atomicAdd(buildRW.traversalInfoWriteCounter, 0);
      if(readIndex < writeCount)
        break;

      if(atomicAdd(buildRW.traversalTaskCounter, 0) == 0)
        return;

      memoryBarrierBuffer();
    }

    if(readIndex >= build.maxTraversalInfos)
      return;

    TraversalInfo task;
    if(waitForTask(readIndex, task))
    {
      processTask(task);
      atomicAdd(buildRW.traversalTaskCounter, -1);
    }
  }
}
