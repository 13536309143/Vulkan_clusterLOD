#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int32 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_buffer_reference2 : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_KHR_shader_subgroup_ballot : require

#if USE_EXT_MESH_SHADER
#extension GL_EXT_mesh_shader : require
#else
#extension GL_NV_mesh_shader : require
#endif

#include "shaderio.h"

layout(scalar, binding = BINDINGS_FRAME_UBO, set = 0) uniform frameConstantsBuffer { FrameConstants view; };
layout(scalar, binding = BINDINGS_READBACK_SSBO, set = 0) buffer readbackBuffer { Readback readback; };
layout(scalar, binding = BINDINGS_RENDERINSTANCES_SSBO, set = 0) buffer renderInstancesBuffer { RenderInstance instances[]; };
layout(scalar, binding = BINDINGS_GEOMETRIES_SSBO, set = 0) buffer geometryBuffer { Geometry geometries[]; };
layout(scalar, binding = BINDINGS_SCENEBUILDING_UBO, set = 0) uniform buildBuffer { SceneBuilding build; };

layout(location = 0) out Interpolants
{
  flat uint clusterID;
  flat uint instanceID;
#if ALLOW_SHADING
  vec3 wPos;
#endif
} OUT[];

#if ALLOW_SHADING && (ALLOW_VERTEX_NORMALS || ALLOW_VERTEX_TEXCOORDS)
layout(location = 3) out Interpolants2
{
  flat uint vertexID;
} OUTBARY[];
#endif

#ifndef MESHSHADER_WORKGROUP_SIZE
#define MESHSHADER_WORKGROUP_SIZE 32
#endif

layout(local_size_x = MESHSHADER_WORKGROUP_SIZE) in;
layout(max_vertices = CLUSTER_VERTEX_COUNT, max_primitives = CLUSTER_TRIANGLE_COUNT) out;
layout(triangles) out;

const uint VERTEX_ITERATIONS = (CLUSTER_VERTEX_COUNT + MESHSHADER_WORKGROUP_SIZE - 1) / MESHSHADER_WORKGROUP_SIZE;
const uint TRIANGLE_ITERATIONS = (CLUSTER_TRIANGLE_COUNT + MESHSHADER_WORKGROUP_SIZE - 1) / MESHSHADER_WORKGROUP_SIZE;

void main()
{
#if USE_EXT_MESH_SHADER
  uint workGroupID = getWorkGroupIndexLinearized(gl_WorkGroupID);
  bool isValid = workGroupID < build.numRenderedClusters;
  ClusterInfo cinfo = build.renderClusterInfos.d[min(workGroupID, build.numRenderedClusters - 1)];
#else
  uint workGroupID = gl_WorkGroupID.x;
  ClusterInfo cinfo = build.renderClusterInfos.d[workGroupID];
  bool isValid = true;
#endif

  uint instanceID = cinfo.instanceID;
  uint clusterID = cinfo.clusterID;
  RenderInstance instance = instances[instanceID];
  Geometry geometry = geometries[instance.geometryID];
  Cluster_in clusterRef = Cluster_in(geometry.preloadedClusters.d[clusterID]);
  Cluster cluster = clusterRef.d;

  uint vertMax = cluster.vertexCountMinusOne;
  uint triMax = cluster.triangleCountMinusOne;

#if USE_EXT_MESH_SHADER
  uint vertCount = isValid ? vertMax + 1 : 0;
  uint triCount = isValid ? triMax + 1 : 0;
  SetMeshOutputsEXT(vertCount, triCount);
  if(triCount == 0)
    return;
#else
  if(gl_LocalInvocationID.x == 0)
    gl_PrimitiveCountNV = triMax + 1;
#endif

#if USE_RENDER_STATS
  if(gl_LocalInvocationID.x == 0)
  {
    atomicAdd(readback.numRenderedTriangles, uint(triMax + 1));
    atomicAdd(readback.numRasteredTriangles, uint(triMax + 1));
  }
#endif

  vec3s_in vertices = Cluster_getVertexPositions(clusterRef);
  uint8s_in localIndices = Cluster_getTriangleIndices(clusterRef);

  for(uint i = 0; i < VERTEX_ITERATIONS; i++)
  {
    uint vert = gl_LocalInvocationID.x + i * MESHSHADER_WORKGROUP_SIZE;
    if(vert <= vertMax)
    {
      vec3 oPos = vertices.d[vert];
      vec3 wPos = instance.worldMatrix * vec4(oPos, 1.0);
      vec4 hPos = view.viewProjMatrix * vec4(wPos, 1.0);

#if USE_EXT_MESH_SHADER
      gl_MeshVerticesEXT[vert].gl_Position = hPos;
#else
      gl_MeshVerticesNV[vert].gl_Position = hPos;
#endif

#if ALLOW_SHADING
      OUT[vert].wPos = wPos;
#endif
#if ALLOW_SHADING && (ALLOW_VERTEX_NORMALS || ALLOW_VERTEX_TEXCOORDS)
      OUTBARY[vert].vertexID = vert;
#endif
      OUT[vert].clusterID = clusterID;
      OUT[vert].instanceID = instanceID;
    }
  }

  for(uint i = 0; i < TRIANGLE_ITERATIONS; i++)
  {
    uint tri = gl_LocalInvocationID.x + i * MESHSHADER_WORKGROUP_SIZE;
    if(tri <= triMax)
    {
      uvec3 indices = uvec3(localIndices.d[tri * 3 + 0], localIndices.d[tri * 3 + 1], localIndices.d[tri * 3 + 2]);
      if(instance.flipWinding != 0)
        indices.xy = indices.yx;

#if USE_EXT_MESH_SHADER
      gl_PrimitiveTriangleIndicesEXT[tri] = indices;
      gl_MeshPrimitivesEXT[tri].gl_PrimitiveID = int(tri);
#else
      gl_PrimitiveIndicesNV[tri * 3 + 0] = indices.x;
      gl_PrimitiveIndicesNV[tri * 3 + 1] = indices.y;
      gl_PrimitiveIndicesNV[tri * 3 + 2] = indices.z;
      gl_MeshPrimitivesNV[tri].gl_PrimitiveID = int(tri);
#endif
    }
  }
}
