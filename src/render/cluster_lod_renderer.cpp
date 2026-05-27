#include <algorithm>
#include <cstring>
#include <vector>

#include <fmt/format.h>
#include <nvutils/alignment.hpp>
#include <volk.h>

#include "render/renderer.hpp"
#include "shaderio.h"

namespace lodclusters {

class RendererRasterClustersLod : public Renderer
{
public:
  bool init(Resources& res, RenderScene& rscene, const RendererConfig& config) override;
  void render(VkCommandBuffer primary, Resources& res, RenderScene& rscene, const FrameConfig& frame,
              nvvk::ProfilerGpuTimer& profiler) override;
  void updatedFrameBuffer(Resources& res, RenderScene& rscene) override;
  void deinit(Resources& res) override;

private:
  bool initShaders(Resources& res, RenderScene& rscene, const RendererConfig& config);
  void updateClusterSelection(Resources& res, const FrameConfig& frame);

  struct Shaders
  {
    shaderc::SpvCompilationResult graphicsMesh;
    shaderc::SpvCompilationResult graphicsFragment;
  };

  struct Pipelines
  {
    VkPipeline graphicsMesh{};
  };

  Shaders                 m_shaders;
  Pipelines               m_pipelines;
  VkShaderStageFlags      m_stageFlags{};
  VkPipelineLayout        m_pipelineLayout{};
  nvvk::DescriptorPack    m_dsetPack;
  nvvk::Buffer            m_sceneBuildBuffer;
  nvvk::Buffer            m_sceneDataBuffer;
  shaderio::SceneBuilding m_sceneBuildShaderio{};
  const RenderScene*      m_renderScene{};
  std::vector<shaderio::ClusterInfo> m_cpuClusterInfos;
  uint32_t                m_frameDataSlots = 4;
};

namespace {
float computeUniformScale(const glm::mat4x3& transform)
{
  return std::max(std::max(glm::length(glm::vec3(transform[0])), glm::length(glm::vec3(transform[1]))),
                  glm::length(glm::vec3(transform[2])));
}

glm::vec3 metricCenter(const shaderio::TraversalMetric& metric)
{
  return {metric.boundingSphereX, metric.boundingSphereY, metric.boundingSphereZ};
}

shaderio::TraversalMetric fallbackMetric(const Scene::GeometryView& geometry, const shaderio::LodLevel& lodLevel)
{
  shaderio::TraversalMetric metric{};
  const glm::vec3           center = (geometry.bbox.lo + geometry.bbox.hi) * 0.5f;
  const glm::vec3           extent = geometry.bbox.hi - geometry.bbox.lo;
  metric.boundingSphereX           = center.x;
  metric.boundingSphereY           = center.y;
  metric.boundingSphereZ           = center.z;
  metric.boundingSphereRadius      = glm::length(extent) * 0.5f;
  metric.maxQuadricError           = lodLevel.minMaxQuadricError;
  return metric;
}
}  // namespace

bool RendererRasterClustersLod::initShaders(Resources& res, RenderScene& rscene, const RendererConfig& config)
{
  if(!initBasicShaders(res, rscene, config))
  {
    return false;
  }

  shaderc::CompileOptions options = res.makeCompilerOptions();
  const uint32_t meshletTriangles = shaderio::adjustClusterProperty(rscene.scene->m_maxClusterTriangles);
  const uint32_t meshletVertices  = shaderio::adjustClusterProperty(rscene.scene->m_maxClusterVertices);

  LOGI("mesh shader config: %d triangles %d vertices\n", meshletTriangles, meshletVertices);

  options.SetOptimizationLevel(shaderc_optimization_level_performance);
  options.AddMacroDefinition("SUBGROUP_SIZE", fmt::format("{}", res.m_physicalDeviceInfo.properties11.subgroupSize));
  options.AddMacroDefinition("USE_16BIT_DISPATCH", fmt::format("{}", res.m_use16bitDispatch ? 1 : 0));
  options.AddMacroDefinition("CLUSTER_VERTEX_COUNT", fmt::format("{}", meshletVertices));
  options.AddMacroDefinition("CLUSTER_TRIANGLE_COUNT", fmt::format("{}", meshletTriangles));
  options.AddMacroDefinition("TARGETS_RASTERIZATION", "1");
  options.AddMacroDefinition("USE_RENDER_STATS", "0");
  options.AddMacroDefinition("USE_EXT_MESH_SHADER", fmt::format("{}", config.useEXTmeshShader ? 1 : 0));
  options.AddMacroDefinition("MESHSHADER_WORKGROUP_SIZE", fmt::format("{}", m_meshShaderWorkgroupSize));
  options.AddMacroDefinition("ALLOW_VERTEX_NORMALS", rscene.scene->m_hasVertexNormals && res.m_supportsBarycentrics ? "1" : "0");
  options.AddMacroDefinition("ALLOW_VERTEX_TANGENTS", rscene.scene->m_hasVertexTangents && res.m_supportsBarycentrics ? "1" : "0");
  options.AddMacroDefinition("ALLOW_VERTEX_TEXCOORDS", rscene.scene->m_hasVertexTexCoord0 ? "1" : "0");
  options.AddMacroDefinition("ALLOW_SHADING", config.useShading ? "1" : "0");
  options.AddMacroDefinition("USE_DEPTH_ONLY", !config.useShading && config.useDepthOnly ? "1" : "0");
  options.AddMacroDefinition("DEBUG_VISUALIZATION", config.useDebugVisualization && res.m_supportsBarycentrics ? "1" : "0");
  options.AddMacroDefinition("USE_TWO_SIDED", "0");
  options.AddMacroDefinition("USE_FORCED_TWO_SIDED", config.forceTwoSided ? "1" : "0");

  res.compileShader(m_shaders.graphicsMesh, VK_SHADER_STAGE_MESH_BIT_NV, "clusters.mesh.glsl", &options);
  res.compileShader(m_shaders.graphicsFragment, VK_SHADER_STAGE_FRAGMENT_BIT, "frag.glsl", &options);

  return res.verifyShaders(m_shaders);
}

bool RendererRasterClustersLod::init(Resources& res, RenderScene& rscene, const RendererConfig& config)
{
  m_resourceReservedUsage = {};
  m_config                = config;
  m_renderScene           = &rscene;
  m_maxTraversalTasks     = 0;

  if(!initShaders(res, rscene, config))
  {
    return false;
  }

  initBasics(res, rscene, config);

  m_maxRenderClusters = 1;
  for(size_t i = 0; i < m_renderInstances.size(); i++)
  {
    const Scene::GeometryView& geometry = rscene.scene->getActiveGeometry(m_renderInstances[i].geometryID);
    uint32_t                   maxGeometryClusters = 0;
    for(const shaderio::LodLevel& lodLevel : geometry.lodLevels)
    {
      maxGeometryClusters = std::max(maxGeometryClusters, lodLevel.clusterCount);
    }
    m_maxRenderClusters += maxGeometryClusters;
  }
  m_cpuClusterInfos.reserve(m_maxRenderClusters);

  m_resourceReservedUsage.geometryMemBytes   = rscene.getGeometrySize();
  m_resourceReservedUsage.operationsMemBytes = logMemoryUsage(rscene.getOperationsSize(), "operations", "rscene total");

  res.createBuffer(m_sceneBuildBuffer, sizeof(shaderio::SceneBuilding),
                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  NVVK_DBG_NAME(m_sceneBuildBuffer.buffer);
  m_resourceReservedUsage.operationsMemBytes += logMemoryUsage(m_sceneBuildBuffer.bufferSize, "operations", "build shaderio");

  memset(&m_sceneBuildShaderio, 0, sizeof(m_sceneBuildShaderio));
  m_sceneBuildShaderio.numRenderInstances = uint32_t(m_renderInstances.size());
  m_sceneBuildShaderio.maxRenderClusters  = m_maxRenderClusters;
  m_sceneBuildShaderio.maxTraversalInfos  = 0;
  m_sceneBuildShaderio.indirectDrawClustersEXT.gridZ = 1;
  m_sceneBuildShaderio.indirectDrawClustersNV.first  = 0;

  BufferRanges mem;
  m_sceneBuildShaderio.renderClusterInfos =
      mem.append(sizeof(shaderio::ClusterInfo) * m_sceneBuildShaderio.maxRenderClusters * m_frameDataSlots, 8);

  res.createBuffer(m_sceneDataBuffer, mem.getSize(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                   VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
                   VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
  NVVK_DBG_NAME(m_sceneDataBuffer.buffer);
  m_resourceReservedUsage.operationsMemBytes += logMemoryUsage(m_sceneDataBuffer.bufferSize, "operations", "build data");

  updateBasicDescriptors(res, rscene, &m_sceneBuildBuffer);

  m_stageFlags = VK_SHADER_STAGE_MESH_BIT_NV | VK_SHADER_STAGE_FRAGMENT_BIT;

  nvvk::DescriptorBindings bindings;
  bindings.addBinding(BINDINGS_FRAME_UBO, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, m_stageFlags);
  bindings.addBinding(BINDINGS_READBACK_SSBO, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, m_stageFlags);
  bindings.addBinding(BINDINGS_GEOMETRIES_SSBO, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, m_stageFlags);
  bindings.addBinding(BINDINGS_RENDERINSTANCES_SSBO, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, m_stageFlags);
  bindings.addBinding(BINDINGS_SCENEBUILDING_SSBO, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, m_stageFlags);
  bindings.addBinding(BINDINGS_SCENEBUILDING_UBO, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, m_stageFlags);
  m_dsetPack.init(bindings, res.m_device);

  nvvk::WriteSetContainer writeSets;
  writeSets.append(m_dsetPack.makeWrite(BINDINGS_FRAME_UBO), res.m_commonBuffers.frameConstants);
  writeSets.append(m_dsetPack.makeWrite(BINDINGS_READBACK_SSBO), res.m_commonBuffers.readBack);
  writeSets.append(m_dsetPack.makeWrite(BINDINGS_GEOMETRIES_SSBO), rscene.getShaderGeometriesBuffer());
  writeSets.append(m_dsetPack.makeWrite(BINDINGS_RENDERINSTANCES_SSBO), m_renderInstanceBuffer);
  writeSets.append(m_dsetPack.makeWrite(BINDINGS_SCENEBUILDING_SSBO), m_sceneBuildBuffer);
  writeSets.append(m_dsetPack.makeWrite(BINDINGS_SCENEBUILDING_UBO), m_sceneBuildBuffer);
  vkUpdateDescriptorSets(res.m_device, writeSets.size(), writeSets.data(), 0, nullptr);

  nvvk::createPipelineLayout(res.m_device, &m_pipelineLayout, {m_dsetPack.getLayout()}, {{m_stageFlags, 0, sizeof(uint32_t)}});

  nvvk::GraphicsPipelineCreator graphicsGen;
  nvvk::GraphicsPipelineState   state              = res.m_basicGraphicsState;
  graphicsGen.pipelineInfo.layout                  = m_pipelineLayout;
  graphicsGen.renderingState.depthAttachmentFormat = res.m_frameBuffer.pipelineRenderingInfo.depthAttachmentFormat;
  graphicsGen.colorFormats                         = {res.m_frameBuffer.colorFormat};
  state.rasterizationState.cullMode                = VK_CULL_MODE_BACK_BIT;

  graphicsGen.addShader(VK_SHADER_STAGE_MESH_BIT_NV, "main", nvvkglsl::GlslCompiler::getSpirvData(m_shaders.graphicsMesh));
  graphicsGen.addShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main", nvvkglsl::GlslCompiler::getSpirvData(m_shaders.graphicsFragment));
  graphicsGen.createGraphicsPipeline(res.m_device, nullptr, state, &m_pipelines.graphicsMesh);

  m_resourceActualUsage = m_resourceReservedUsage;
  return true;
}

void RendererRasterClustersLod::updateClusterSelection(Resources& res, const FrameConfig& frame)
{
  m_cpuClusterInfos.clear();

  if(!m_renderScene || !m_renderScene->scene)
  {
    return;
  }

  const float threshold =
      std::max(1.0e-6f, clusterLodErrorOverDistance(frame.lodPixelError, frame.frameConstants.fov, frame.frameConstants.viewportf.y));
  const float nearPlane = std::max(1.0e-6f, frame.frameConstants.nearPlane);
  const glm::vec3 viewPos = glm::vec3(frame.frameConstants.viewPos);

  for(size_t instanceIndex = 0; instanceIndex < m_renderInstances.size(); instanceIndex++)
  {
    const shaderio::RenderInstance& renderInstance = m_renderInstances[instanceIndex];
    const Scene::GeometryView& geometry = m_renderScene->scene->getActiveGeometry(renderInstance.geometryID);
    if(geometry.lodLevels.empty())
    {
      continue;
    }

    const float uniformScale = computeUniformScale(renderInstance.worldMatrix);
    uint32_t selectedLevel = uint32_t(geometry.lodLevels.size() - 1);

    for(int level = int(geometry.lodLevels.size()) - 1; level >= 0; level--)
    {
      const shaderio::LodLevel& lodLevel = geometry.lodLevels[size_t(level)];
      const shaderio::TraversalMetric metric =
          geometry.lodNodes.size() > size_t(level + 1) ? geometry.lodNodes[size_t(level + 1)].traversalMetric :
                                                         fallbackMetric(geometry, lodLevel);

      const glm::vec3 objectCenter = metricCenter(metric);
      const glm::vec3 worldCenter  = renderInstance.worldMatrix * glm::vec4(objectCenter, 1.0f);
      const float distance         = glm::length(viewPos - worldCenter);
      const float errorDistance    = std::max(nearPlane, distance - metric.boundingSphereRadius * uniformScale);
      const float errorOverDistance = metric.maxQuadricError * uniformScale / errorDistance;

      if(errorOverDistance <= threshold || level == 0)
      {
        selectedLevel = uint32_t(level);
        break;
      }
    }

    const shaderio::LodLevel& selectedLod = geometry.lodLevels[selectedLevel];
    for(uint32_t c = 0; c < selectedLod.clusterCount; c++)
    {
      shaderio::ClusterInfo info{};
      info.instanceID = uint32_t(instanceIndex);
      info.clusterID  = selectedLod.clusterOffset + c;
      m_cpuClusterInfos.push_back(info);
    }
  }

  if(!m_cpuClusterInfos.empty())
  {
    const uint32_t slot = res.m_cycleIndex % m_frameDataSlots;
    const VkDeviceSize slotSize = sizeof(shaderio::ClusterInfo) * m_sceneBuildShaderio.maxRenderClusters;
    const VkDeviceSize offset   = slotSize * slot;
    memcpy(m_sceneDataBuffer.mapping + offset, m_cpuClusterInfos.data(), sizeof(shaderio::ClusterInfo) * m_cpuClusterInfos.size());
    NVVK_CHECK(res.m_allocator.autoFlushBuffer(m_sceneDataBuffer, offset,
                                               sizeof(shaderio::ClusterInfo) * m_cpuClusterInfos.size()));
    m_sceneBuildShaderio.renderClusterInfos = m_sceneDataBuffer.address + offset;
  }
  else
  {
    m_sceneBuildShaderio.renderClusterInfos = m_sceneDataBuffer.address;
  }
}

void RendererRasterClustersLod::render(VkCommandBuffer cmd, Resources& res, RenderScene& rscene, const FrameConfig& frame,
                                       nvvk::ProfilerGpuTimer& profiler)
{
  (void)profiler;
  updateClusterSelection(res, frame);

  VkMemoryBarrier memBarrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};

  m_sceneBuildShaderio.pass                     = 0;
  m_sceneBuildShaderio.renderClusterCounter     = uint32_t(m_cpuClusterInfos.size());
  m_sceneBuildShaderio.traversalTaskCounter     = 0;
  m_sceneBuildShaderio.traversalInfoReadCounter = 0;
  m_sceneBuildShaderio.traversalInfoWriteCounter = 0;
  m_sceneBuildShaderio.numRenderedClusters      = uint32_t(m_cpuClusterInfos.size());
  m_sceneBuildShaderio.indirectDrawClustersNV.count = uint32_t(m_cpuClusterInfos.size());
  m_sceneBuildShaderio.indirectDrawClustersNV.first = 0;
  m_sceneBuildShaderio.indirectDrawClustersEXT.gridX = uint32_t(m_cpuClusterInfos.size());
  m_sceneBuildShaderio.indirectDrawClustersEXT.gridY = 1;
  m_sceneBuildShaderio.indirectDrawClustersEXT.gridZ = 1;
  m_sceneBuildShaderio.numRenderInstances        = uint32_t(m_renderInstances.size());
  m_sceneBuildShaderio.errorOverDistanceThreshold =
      clusterLodErrorOverDistance(frame.lodPixelError, frame.frameConstants.fov, frame.frameConstants.viewportf.y);
  m_sceneBuildShaderio.traversalViewMatrix = frame.traversalViewMatrix;
  m_sceneBuildShaderio.frameIndex          = m_frameIndex;

  vkCmdUpdateBuffer(cmd, res.m_commonBuffers.frameConstants.buffer, 0, sizeof(shaderio::FrameConstants),
                    (const uint32_t*)&frame.frameConstants);
  vkCmdUpdateBuffer(cmd, m_sceneBuildBuffer.buffer, 0, sizeof(shaderio::SceneBuilding), (const uint32_t*)&m_sceneBuildShaderio);

  memBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_HOST_WRITE_BIT;
  memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT;
  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 1, &memBarrier,
                       0, nullptr, 0, nullptr);

  {
    res.cmdBeginRendering(cmd);
    if(m_config.useShading)
    {
      writeBackgroundSky(cmd);
    }

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, m_dsetPack.getSetPtr(), 0, nullptr);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines.graphicsMesh);
    const uint32_t meshTaskCount = uint32_t(m_cpuClusterInfos.size());
    if(meshTaskCount && m_config.useEXTmeshShader)
    {
      vkCmdDrawMeshTasksEXT(cmd, meshTaskCount, 1, 1);
    }
    else if(meshTaskCount)
    {
      vkCmdDrawMeshTasksNV(cmd, meshTaskCount, 0);
    }
    vkCmdEndRendering(cmd);
  }

  m_resourceReservedUsage.geometryMemBytes = rscene.getGeometrySize();
  m_resourceActualUsage                    = m_resourceReservedUsage;
  m_frameIndex++;
}

void RendererRasterClustersLod::updatedFrameBuffer(Resources& res, RenderScene& rscene)
{
  vkDeviceWaitIdle(res.m_device);
  Renderer::updatedFrameBuffer(res, rscene);
}

void RendererRasterClustersLod::deinit(Resources& res)
{
  res.destroyPipelines(m_pipelines);
  vkDestroyPipelineLayout(res.m_device, m_pipelineLayout, nullptr);
  m_dsetPack.deinit();
  res.m_allocator.destroyBuffer(m_sceneDataBuffer);
  res.m_allocator.destroyBuffer(m_sceneBuildBuffer);
  deinitBasics(res);
}

std::unique_ptr<Renderer> makeRendererRasterClustersLod()
{
  return std::make_unique<RendererRasterClustersLod>();
}

}  // namespace lodclusters
