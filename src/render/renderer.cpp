#include <algorithm>
#include <vector>

#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/packing.hpp>
#include <volk.h>

#include "render/renderer.hpp"
#include "shaderio.h"

namespace lodclusters {

bool RenderScene::init(Resources* res, const Scene* scene_)
{
  scene = scene_;

  ScenePreloaded::Config preloadConfig;
  if(!scenePreloaded.init(res, scene_, preloadConfig))
  {
    scenePreloaded.deinit();
    scene = nullptr;
    return false;
  }

  return true;
}

void RenderScene::deinit()
{
  scenePreloaded.deinit();
}

const nvvk::BufferTyped<shaderio::Geometry>& RenderScene::getShaderGeometriesBuffer() const
{
  return scenePreloaded.getShaderGeometriesBuffer();
}

size_t RenderScene::getGeometrySize() const
{
  return scenePreloaded.getGeometrySize();
}

size_t RenderScene::getOperationsSize() const
{
  return scenePreloaded.getOperationsSize();
}

bool Renderer::initBasicShaders(Resources& res, RenderScene&, const RendererConfig& config)
{
  if(config.useEXTmeshShader)
  {
    m_meshShaderWorkgroupSize = std::min(128u, std::min(res.m_meshShaderPropsEXT.maxPreferredMeshWorkGroupInvocations,
                                                        std::min(res.m_meshShaderPropsEXT.maxMeshWorkGroupSize[0],
                                                                 res.m_meshShaderPropsEXT.maxMeshWorkGroupInvocations)));
  }
  else
  {
    m_meshShaderWorkgroupSize = 32u;
  }

  res.compileShader(m_basicShaders.fullScreenVertexShader, VK_SHADER_STAGE_VERTEX_BIT, "fullscreen.vert.glsl");
  res.compileShader(m_basicShaders.fullScreenBackgroundFragShader, VK_SHADER_STAGE_FRAGMENT_BIT,
                    "fullscreen_background.frag.glsl");

  return res.verifyShaders(m_basicShaders);
}

void Renderer::initBasics(Resources& res, RenderScene& rscene, const RendererConfig& config)
{
  initBasicPipelines(res, rscene, config);

  const Scene& scene = *rscene.scene;
  m_renderInstances.resize(scene.m_instances.size());

  for(size_t i = 0; i < m_renderInstances.size(); i++)
  {
    shaderio::RenderInstance&  renderInstance = m_renderInstances[i];
    const Scene::Instance&     sceneInstance  = scene.m_instances[i];
    const Scene::GeometryView& geometry       = scene.getActiveGeometry(sceneInstance.geometryID);

    renderInstance                = {};
    renderInstance.worldMatrix    = glm::mat4x3(sceneInstance.matrix);
    renderInstance.worldMatrixI   = glm::mat4x3(glm::inverse(sceneInstance.matrix));
    renderInstance.geometryID     = sceneInstance.geometryID;
    renderInstance.materialID     = uint16_t(sceneInstance.materialID);
    renderInstance.maxLodLevelRcp = geometry.lodLevelsCount > 1 ? 1.0f / float(geometry.lodLevelsCount - 1) : 0.0f;
    renderInstance.packedColor    = glm::packUnorm4x8(sceneInstance.color);
    renderInstance.twoSided       = sceneInstance.twoSided ? 1 : 0;
    renderInstance.flipWinding =
        (!sceneInstance.twoSided && ((glm::determinant(sceneInstance.matrix) <= 0) != config.flipWinding)) ? 1 : 0;
  }

  res.createBuffer(m_renderInstanceBuffer, sizeof(shaderio::RenderInstance) * m_renderInstances.size(),
                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  NVVK_DBG_NAME(m_renderInstanceBuffer.buffer);
  res.simpleUploadBuffer(m_renderInstanceBuffer, m_renderInstances.data());
}

void Renderer::deinitBasics(Resources& res)
{
  res.destroyPipelines(m_basicPipelines);
  vkDestroyPipelineLayout(res.m_device, m_basicPipelineLayout, nullptr);
  m_basicDset.deinit();
  res.m_allocator.destroyBuffer(m_renderInstanceBuffer);
}

void Renderer::updateBasicDescriptors(Resources& res, RenderScene& rscene, const nvvk::Buffer* sceneBuildBuffer)
{
  nvvk::WriteSetContainer writeSets;
  writeSets.append(m_basicDset.makeWrite(BINDINGS_FRAME_UBO), res.m_commonBuffers.frameConstants);
  writeSets.append(m_basicDset.makeWrite(BINDINGS_READBACK_SSBO), res.m_commonBuffers.readBack);
  writeSets.append(m_basicDset.makeWrite(BINDINGS_GEOMETRIES_SSBO), rscene.getShaderGeometriesBuffer());
  writeSets.append(m_basicDset.makeWrite(BINDINGS_RENDERINSTANCES_SSBO), m_renderInstanceBuffer);
  if(sceneBuildBuffer)
  {
    writeSets.append(m_basicDset.makeWrite(BINDINGS_SCENEBUILDING_UBO), *sceneBuildBuffer);
  }
  vkUpdateDescriptorSets(res.m_device, writeSets.size(), writeSets.data(), 0, nullptr);
}

void Renderer::initBasicPipelines(Resources& res, RenderScene&, const RendererConfig&)
{
  m_basicShaderFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

  nvvk::DescriptorBindings bindings;
  bindings.addBinding(BINDINGS_FRAME_UBO, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, m_basicShaderFlags);
  bindings.addBinding(BINDINGS_READBACK_SSBO, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, m_basicShaderFlags);
  bindings.addBinding(BINDINGS_GEOMETRIES_SSBO, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, m_basicShaderFlags);
  bindings.addBinding(BINDINGS_RENDERINSTANCES_SSBO, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, m_basicShaderFlags);
  bindings.addBinding(BINDINGS_SCENEBUILDING_UBO, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, m_basicShaderFlags);
  m_basicDset.init(bindings, res.m_device);

  nvvk::createPipelineLayout(res.m_device, &m_basicPipelineLayout, {m_basicDset.getLayout()},
                             {{m_basicShaderFlags, 0, sizeof(uint32_t)}});

  nvvk::GraphicsPipelineCreator graphicsGen;
  nvvk::GraphicsPipelineState   state              = res.m_basicGraphicsState;
  graphicsGen.pipelineInfo.layout                  = m_basicPipelineLayout;
  graphicsGen.renderingState.depthAttachmentFormat = res.m_frameBuffer.pipelineRenderingInfo.depthAttachmentFormat;
  graphicsGen.colorFormats                         = {res.m_frameBuffer.colorFormat};
  state.depthStencilState.depthWriteEnable         = VK_TRUE;
  state.depthStencilState.depthCompareOp           = VK_COMPARE_OP_ALWAYS;
  state.rasterizationState.cullMode                = VK_CULL_MODE_NONE;

  graphicsGen.addShader(VK_SHADER_STAGE_VERTEX_BIT, "main",
                        nvvkglsl::GlslCompiler::getSpirvData(m_basicShaders.fullScreenVertexShader));
  graphicsGen.addShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main",
                        nvvkglsl::GlslCompiler::getSpirvData(m_basicShaders.fullScreenBackgroundFragShader));
  graphicsGen.createGraphicsPipeline(res.m_device, nullptr, state, &m_basicPipelines.background);
}

void Renderer::writeBackgroundSky(VkCommandBuffer cmd)
{
  uint32_t dummy = 0;
  vkCmdPushConstants(cmd, m_basicPipelineLayout, m_basicShaderFlags, 0, sizeof(uint32_t), &dummy);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_basicPipelineLayout, 0, 1, m_basicDset.getSetPtr(), 0,
                          nullptr);
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_basicPipelines.background);
  vkCmdDraw(cmd, 3, 1, 0, 0);
}

}  // namespace lodclusters
