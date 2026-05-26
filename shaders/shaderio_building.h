#ifndef _SHADERIO_BUILDING_H_
#define _SHADERIO_BUILDING_H_

#define TRAVERSAL_INVALID_LOD_LEVEL 0xFF

#ifdef __cplusplus
namespace shaderio {
using namespace glm;
#endif

// One queue item in the cluster LOD traversal. It can encode either a LOD node
// or a low-detail cluster group for one render instance.
struct TraversalInfo
{
  uint32_t instanceID;
  uint32_t packedNode;
};

// One cluster selected by traversal and consumed by the mesh shader.
// This intentionally matches TraversalInfo's 64-bit footprint.
struct ClusterInfo
{
  uint32_t instanceID;
  uint32_t clusterID;
};
BUFFER_REF_DECLARE_ARRAY(ClusterInfos_inout, ClusterInfo, , 8);

// Runtime state shared between traversal/build setup shaders and the mesh draw.
struct SceneBuilding
{
  mat4 traversalViewMatrix;

  uint pass;
  uint frameIndex;

  uint numRenderInstances;
  uint maxRenderClusters;
  uint maxTraversalInfos;

  float errorOverDistanceThreshold;

  uint renderClusterCounter;

  int  traversalTaskCounter;
  uint traversalInfoReadCounter;
  uint traversalInfoWriteCounter;

  BUFFER_REF(uint64s_coh_volatile) traversalNodeInfos;
  BUFFER_REF(ClusterInfos_inout) renderClusterInfos;

  DrawMeshTasksIndirectCommandNV indirectDrawClustersNV;

  DrawMeshTasksIndirectCommandEXT indirectDrawClustersEXT;
  uint                            numRenderedClusters;
};

#ifdef __cplusplus
}
#endif
#endif  // _SHADERIO_BUILDING_H_
