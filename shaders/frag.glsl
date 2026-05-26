#version 460
#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int8 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int32 : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_buffer_reference2 : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_shader_atomic_int64 : enable

#if ALLOW_SHADING && (DEBUG_VISUALIZATION || ALLOW_VERTEX_NORMALS || ALLOW_VERTEX_TEXCOORDS)
#extension GL_EXT_fragment_shader_barycentric : enable
#endif

#include "shaderio.h"
#include "attribute_encoding.h"

layout(scalar, binding = BINDINGS_FRAME_UBO, set = 0) uniform frameConstantsBuffer { FrameConstants view; };
layout(scalar, binding = BINDINGS_READBACK_SSBO, set = 0) buffer readbackBuffer { Readback readback; };
layout(scalar, binding = BINDINGS_RENDERINSTANCES_SSBO, set = 0) buffer renderInstancesBuffer { RenderInstance instances[]; };
layout(scalar, binding = BINDINGS_GEOMETRIES_SSBO, set = 0) buffer geometryBuffer { Geometry geometries[]; };

vec3 hue2rgb(float hue)
{
  hue = fract(hue);
  return clamp(vec3(abs(hue * 6.0 - 3.0) - 1.0, 2.0 - abs(hue * 6.0 - 2.0), 2.0 - abs(hue * 6.0 - 4.0)),
               vec3(0.0), vec3(1.0));
}

vec3 lodMix(float v)
{
  float low = 0.15;
  if(v == 0.0)
    return vec3(1.0);
  if(v < low)
    return mix(vec3(1.0), hue2rgb(0.5), v / low);

  v = (v - low) / (1.0 - low);
  return hue2rgb(0.5 - v * 0.5);
}

vec3 colorizeID(uint id)
{
  return unpackUnorm4x8(murmurHash(id ^ view.colorXor)).xyz * 0.5 + 0.3;
}

vec3 visualizeColor(uint visData, uint instanceID)
{
  if(view.visualize == VISUALIZE_LOD)
    return lodMix(uintBitsToFloat(visData)) * 0.7 + 0.2;
  if(view.visualize == VISUALIZE_MATERIAL)
    return pow(unpackUnorm4x8(instances[instanceID].packedColor).xyz * 0.95 + 0.05, vec3(1.0 / 2.2));
  if(view.visualize == VISUALIZE_GREY)
    return vec3(0.82);

  return colorizeID(visData);
}

vec4 shadeCluster(uint instanceID, vec3 wPos, vec3 wNormal, uint visData)
{
  vec3 albedo = visualizeColor(visData, instanceID);
  vec3 normal = normalize(wNormal);
  vec3 lightDir = normalize(view.wLightPos.xyz - wPos);
  vec3 viewDir = normalize(view.viewMatrixI[3].xyz - wPos);
  vec3 halfDir = normalize(lightDir + viewDir);

  float diffuse = max(dot(normal, lightDir), 0.0);
  float specular = pow(max(dot(normal, halfDir), 0.0), 24.0) * 0.18;
  vec3 ambient = albedo * 0.35;
  vec3 lit = ambient + albedo * diffuse * 0.75 + vec3(specular);
  return vec4(lit, 1.0);
}

layout(location = 0) in Interpolants
{
  flat uint clusterID;
  flat uint instanceID;
#if ALLOW_SHADING
  vec3 wPos;
#endif
} IN;

#if ALLOW_SHADING && (ALLOW_VERTEX_NORMALS || ALLOW_VERTEX_TEXCOORDS)
layout(location = 3) pervertexEXT in Interpolants2
{
  uint vertexID;
} INBARY[];
#endif

layout(location = 0, index = 0) out vec4 out_Color;
layout(early_fragment_tests) in;

void main()
{
  RenderInstance instance = instances[IN.instanceID];
  Geometry geometry = geometries[instance.geometryID];
  Cluster_in clusterRef = Cluster_in(geometry.preloadedClusters.d[IN.clusterID]);

  uint visData = IN.clusterID;

#if ALLOW_SHADING
  Cluster cluster = clusterRef.d;
  vec4 wTangent = vec4(1.0);
  vec3 wNormal = vec3(1.0);
  vec2 oTexCoord = vec2(1.0);

#if ALLOW_VERTEX_NORMALS || ALLOW_VERTEX_TEXCOORDS
  uint32s_in oNormals = Cluster_getVertexNormals(clusterRef);
  vec2s_in oTexCoords = Cluster_getVertexTexCoords(clusterRef);
  uvec3 triangleIndices = uvec3(INBARY[0].vertexID, INBARY[1].vertexID, INBARY[2].vertexID);
#endif

#if ALLOW_VERTEX_NORMALS
  if(view.facetShading != 0 || (cluster.attributeBits & CLUSTER_ATTRIBUTE_VERTEX_NORMAL) == 0)
#endif
  {
    wNormal = normalize(-cross(dFdx(IN.wPos), dFdy(IN.wPos)));
  }

#if ALLOW_VERTEX_NORMALS
  else
  {
    vec3 baryWeight = gl_BaryCoordEXT;
    mat3 worldMatrixI = mat3(instance.worldMatrixI);
    uvec3 packed = uvec3(oNormals.d[triangleIndices.x], oNormals.d[triangleIndices.y], oNormals.d[triangleIndices.z]);
    vec3 n0 = normal_unpack(packed.x);
    vec3 n1 = normal_unpack(packed.y);
    vec3 n2 = normal_unpack(packed.z);
    vec3 oNormal = baryWeight.x * n0 + baryWeight.y * n1 + baryWeight.z * n2;
    wNormal = normalize(vec3(oNormal * worldMatrixI));
  }
#endif

#if ALLOW_VERTEX_TEXCOORDS
  if((cluster.attributeBits & CLUSTER_ATTRIBUTE_VERTEX_TEX_0) != 0)
  {
    oTexCoord = gl_BaryCoordEXT.x * oTexCoords.d[triangleIndices.x]
              + gl_BaryCoordEXT.y * oTexCoords.d[triangleIndices.y]
              + gl_BaryCoordEXT.z * oTexCoords.d[triangleIndices.z];
  }
#endif

  if(view.visualize == VISUALIZE_LOD)
    visData = floatBitsToUint(float(cluster.lodLevel) * instances[IN.instanceID].maxLodLevelRcp);
  else if(view.visualize == VISUALIZE_GROUP)
  {
    uvec2 baseAddress = unpackUint2x32(uint64_t(clusterRef) - cluster.groupChildIndex * Cluster_size);
    visData = baseAddress.x ^ baseAddress.y;
  }
  else if(view.visualize == VISUALIZE_TRIANGLE)
    visData = IN.clusterID * 256 + uint(gl_PrimitiveID);

  out_Color = shadeCluster(IN.instanceID, IN.wPos, wNormal, visData);
#else
  float relative = (float(gl_PrimitiveID) / float(CLUSTER_TRIANGLE_COUNT - 1)) * 0.25 + 0.75;
  out_Color = vec4(colorizeID(visData) * relative, 1.0);
#endif

}
