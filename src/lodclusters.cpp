#include <exception>

#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui/imgui.h>
#include <nvgui/camera.hpp>
#include <nvgui/file_dialog.hpp>
#include <nvgui/property_editor.hpp>
#include <nvgui/sky.hpp>
#include <volk.h>

#include "lodclusters.hpp"

bool g_verbose = false;

namespace lodclusters {

static std::string formatMetric(size_t size)
{
  return fmt::format("{}", size);
}

std::string formatMemorySize(size_t sizeInBytes)
{
  static const std::string units[]     = {"B", "KiB", "MiB", "GiB"};
  static const size_t      unitSizes[] = {1, 1024, 1024 * 1024, 1024ull * 1024ull * 1024ull};

  uint32_t currentUnit = 0;
  for(uint32_t i = 1; i < 4; i++)
  {
    if(sizeInBytes < unitSizes[i])
      break;
    currentUnit++;
  }

  return fmt::format("{:.3} {}", float(sizeInBytes) / float(unitSizes[currentUnit]), units[currentUnit]);
}

LodClusters::LodClusters(const Info& info)
    : m_info(info)
{
  nvutils::ProfilerTimeline::CreateInfo createInfo;
  createInfo.name    = "graphics";
  m_profilerTimeline = m_info.profilerManager->createTimeline(createInfo);

  m_info.parameterRegistry->add({"scene"}, {".gltf", ".glb"}, &m_sceneFilePathDropNew);
  m_info.parameterRegistry->add({"verbose"}, &g_verbose, true);
  m_info.parameterRegistry->add({"supersample"}, &m_tweak.supersample);
  m_info.parameterRegistry->add({"clusterconfig"}, (int*)&m_tweak.clusterConfig);
  m_info.parameterRegistry->add({"clustergroupsize"}, &m_sceneConfig.clusterGroupSize);
  m_info.parameterRegistry->add({"lodnodewidth"}, &m_sceneConfig.preferredNodeWidth);
  m_info.parameterRegistry->add({"loddecimationfactor"}, &m_sceneConfig.lodLevelDecimationFactor);
  m_info.parameterRegistry->add({"loderror"}, &m_frameConfig.lodPixelError);
  m_info.parameterRegistry->add({"renderclusterbits"}, &m_rendererConfig.numRenderClusterBits);
  m_info.parameterRegistry->add({"rendertraversalbits"}, &m_rendererConfig.numTraversalTaskBits);
  m_info.parameterRegistry->add({"visualize"}, &m_frameConfig.visualize);
  m_info.parameterRegistry->add({"renderstats"}, &m_rendererConfig.useRenderStats);
  m_info.parameterRegistry->add({"extmeshshader"}, &m_rendererConfig.useEXTmeshShader);
  m_info.parameterRegistry->add({"facetshading"}, &m_tweak.facetShading);
  m_info.parameterRegistry->add({"flipwinding"}, &m_rendererConfig.flipWinding);
  m_info.parameterRegistry->add({"forcetwosided"}, &m_rendererConfig.forceTwoSided);
  m_info.parameterRegistry->add({"camerastring"}, &m_cameraString);
  m_info.parameterRegistry->add({"cameraspeed"}, &m_cameraSpeed);
  m_info.parameterRegistry->add({"dumpspirv"}, &m_resources.m_dumpSpirv);
  m_info.parameterRegistry->add({"attributes"}, &m_sceneConfig.enabledAttributes);
  m_info.parameterRegistry->add({"compressed"}, &m_sceneConfig.useCompressedData);

  m_frameConfig.frameConstants                         = {};
  m_frameConfig.lodPixelError                          = 8.0f;
  m_frameConfig.frameConstants.wireThickness           = 2.0f;
  m_frameConfig.frameConstants.wireSmoothing           = 1.0f;
  m_frameConfig.frameConstants.wireColor               = {118.f / 255.f, 185.f / 255.f, 0.f};
  m_frameConfig.frameConstants.wireBackfaceColor       = {0.5f, 0.5f, 0.5f};
  m_frameConfig.frameConstants.wireStippleRepeats      = 5.0f;
  m_frameConfig.frameConstants.wireStippleLength       = 0.5f;
  m_frameConfig.frameConstants.doShadow                = 1;
  m_frameConfig.frameConstants.ambientOcclusionRadius  = 0.1f;
  m_frameConfig.frameConstants.ambientOcclusionSamples = 2;
  m_frameConfig.frameConstants.visualize               = VISUALIZE_LOD;
  m_frameConfig.frameConstants.facetShading            = 1;
  m_frameConfig.frameConstants.lightMixer              = 0.5f;
  m_frameConfig.frameConstants.skyParams               = {};
}

LodClusters::~LodClusters()
{
  if(m_profilerTimeline)
  {
    m_info.profilerManager->destroyTimeline(m_profilerTimeline);
  }
}

void LodClusters::onAttach(nvapp::Application* app)
{
  m_app = app;
  m_info.cameraManipulator->setMode(nvutils::CameraManipulator::Fly);

  if(m_resources.m_supportsSmBuiltinsNV)
  {
    VkPhysicalDeviceProperties2 physicalProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    VkPhysicalDeviceShaderSMBuiltinsPropertiesNV smProperties{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_PROPERTIES_NV};
    physicalProperties.pNext = &smProperties;
    vkGetPhysicalDeviceProperties2(app->getPhysicalDevice(), &physicalProperties);
    m_frameConfig.traversalPersistentThreads = std::max(2048u, smProperties.shaderSMCount * smProperties.shaderWarpsPerSM * 4);
  }

  for(uint32_t i = 0; i < NUM_CLUSTER_CONFIGS; i++)
  {
    m_ui.enumAdd(GUI_MESHLET, s_clusterInfos[i].cfg, fmt::format("{}T_{}V", s_clusterInfos[i].tris, s_clusterInfos[i].verts).c_str());
  }
  m_ui.enumAdd(GUI_SUPERSAMPLE, 1, "none");
  m_ui.enumAdd(GUI_SUPERSAMPLE, 2, "4x");
  m_ui.enumAdd(GUI_SUPERSAMPLE, 720, "720p");
  m_ui.enumAdd(GUI_SUPERSAMPLE, 1080, "1080p");
  m_ui.enumAdd(GUI_SUPERSAMPLE, 1440, "1440p");
  m_ui.enumAdd(GUI_SUPERSAMPLE, 2160, "2160p");
  m_ui.enumAdd(GUI_VISUALIZE, VISUALIZE_MATERIAL, "material");
  m_ui.enumAdd(GUI_VISUALIZE, VISUALIZE_GREY, "grey");
  m_ui.enumAdd(GUI_VISUALIZE, VISUALIZE_CLUSTER, "clusters");
  m_ui.enumAdd(GUI_VISUALIZE, VISUALIZE_GROUP, "cluster groups");
  m_ui.enumAdd(GUI_VISUALIZE, VISUALIZE_LOD, "lod levels");
  m_ui.enumAdd(GUI_VISUALIZE, VISUALIZE_TRIANGLE, "triangles");

  m_resources.init(app->getDevice(), app->getPhysicalDevice(), app->getInstance(), app->getQueue(0), app->getQueue(1));

  NVVK_CHECK(m_resources.m_samplerPool.acquireSampler(m_imguiSampler));
  NVVK_DBG_NAME(m_imguiSampler);

  m_windowSize = {128, 128};
  m_resources.initFramebuffer(m_windowSize, m_tweak.supersample);
  updateImguiImage();
  setFromClusterConfig(m_sceneConfig, m_tweak.clusterConfig);

  if(!m_resources.m_supportsMeshShaderNV)
  {
    m_rendererConfig.useEXTmeshShader = true;
  }

  if(!m_sceneFilePathDropNew.empty())
  {
    onFileDrop(m_sceneFilePathDropNew);
  }
  else
  {
    LOGI("No scene loaded. Use --scene <file.glb> or File/Open.\n");
  }

  m_tweakLast          = m_tweak;
  m_sceneConfigLast    = m_sceneConfig;
  m_rendererConfigLast = m_rendererConfig;
}

void LodClusters::onDetach()
{
  NVVK_CHECK(vkDeviceWaitIdle(m_app->getDevice()));
  deinitRenderer();
  deinitScene();
  m_resources.m_samplerPool.releaseSampler(m_imguiSampler);
  if(m_imguiTexture)
  {
    ImGui_ImplVulkan_RemoveTexture(m_imguiTexture);
  }
  m_resources.deinit();
}

void LodClusters::initScene(const std::filesystem::path& filePath, bool configChange)
{
  deinitScene();
  if(filePath.empty())
    return;

  m_sceneLoading  = true;
  m_sceneProgress = 0;

  try
  {
    auto scene = std::make_unique<Scene>();
    SceneLoaderConfig loaderConfig;
    loaderConfig.processingThreadsPct = 0.05f;
    loaderConfig.forcePreprocessMiB   = SIZE_MAX;
    loaderConfig.progressPct          = &m_sceneProgress;

    Scene::Result result = scene->init(filePath, m_sceneConfig, loaderConfig);
    if(result != Scene::SCENE_RESULT_SUCCESS)
    {
      LOGW("Loading scene failed, result %d\n", int(result));
      m_sceneLoading = false;
      return;
    }

    LOGI("Scene CPU load complete; initializing GPU resources\n");
    m_scene               = std::move(scene);
    m_sceneFilePath       = filePath;
    m_tweak.clusterConfig = findSceneClusterConfig(m_scene->m_config);
    if(!configChange)
    {
      m_sceneConfig     = m_scene->m_config;
      m_sceneConfigLast = m_sceneConfig;
    }
    postInitNewScene();
  }
  catch(const std::exception& e)
  {
    LOGE("Loading scene crashed: %s\n", e.what());
  }
  catch(...)
  {
    LOGE("Loading scene crashed: unknown exception\n");
  }

  m_sceneLoading = false;
}

void LodClusters::deinitScene()
{
  deinitRenderScene();
  if(m_scene)
  {
    m_scene->deinit();
    m_scene = nullptr;
  }
}

void LodClusters::initRenderScene()
{
  if(!m_scene)
    return;

  LOGI("Creating preloaded GPU scene\n");
  m_renderScene = std::make_unique<RenderScene>();
  if(!m_renderScene->init(&m_resources, m_scene.get()))
  {
    LOGW("Init RenderScene failed\n");
    m_renderScene = nullptr;
    return;
  }
  LOGI("Preloaded GPU scene created\n");
}

void LodClusters::deinitRenderScene()
{
  if(m_app)
  {
    NVVK_CHECK(vkDeviceWaitIdle(m_app->getDevice()));
  }
  if(m_renderScene)
  {
    m_renderScene->deinit();
    m_renderScene = nullptr;
  }
}

void LodClusters::initRenderer()
{
  deinitRenderer();
  if(!m_renderScene)
    return;

  m_renderer = makeRendererRasterClustersLod();
  LOGI("Creating cluster LOD renderer\n");
  if(!m_renderer->init(m_resources, *m_renderScene, m_rendererConfig))
  {
    LOGE("Renderer init failed\n");
    m_renderer = nullptr;
    return;
  }
  m_rendererFboChangeID = m_resources.m_fboChangeID;
  LOGI("Cluster LOD renderer ready\n");
}

void LodClusters::deinitRenderer()
{
  if(m_app)
  {
    NVVK_CHECK(vkDeviceWaitIdle(m_app->getDevice()));
  }
  if(m_renderer)
  {
    m_renderer->deinit(m_resources);
    m_renderer = nullptr;
  }
}

void LodClusters::postInitNewScene()
{
  glm::vec3 extent         = m_scene->m_bbox.hi - m_scene->m_bbox.lo;
  glm::vec3 center         = (m_scene->m_bbox.hi + m_scene->m_bbox.lo) * 0.5f;
  float     sceneDimension = glm::length(extent);

  m_frameConfig.frameConstants.wLightPos = center + sceneDimension;
  m_frameConfig.frameConstants.sceneSize = sceneDimension;
  if(!m_scene->m_hasVertexNormals)
  {
    m_tweak.facetShading = true;
  }
  setSceneCamera();
  LOGI("Initializing GPU resources for preloaded cluster LOD scene\n");
  initRenderScene();
  initRenderer();
  m_frames = 0;
}

void LodClusters::setSceneCamera()
{
  glm::vec3 modelExtent = m_scene->m_bbox.hi - m_scene->m_bbox.lo;
  float     modelRadius = glm::length(modelExtent) * 0.5f;
  glm::vec3 modelCenter = (m_scene->m_bbox.hi + m_scene->m_bbox.lo) * 0.5f;

  if(!m_scene->m_cameras.empty())
  {
    const auto& c = m_scene->m_cameras[0];
    glm::vec3 eye = glm::vec3(c.worldMatrix[3]);
    float distance = glm::length(modelCenter - eye);
    glm::vec3 center = eye + glm::mat3(c.worldMatrix) * glm::vec3(0, 0, -distance);
    m_info.cameraManipulator->setCamera({eye, center, {0, 1, 0}, static_cast<float>(glm::degrees(c.fovy))});
  }
  else
  {
    m_info.cameraManipulator->setLookat(modelCenter + glm::vec3(1.0f, 0.75f, 1.0f) * modelRadius, modelCenter, {0, 1, 0});
  }
  m_info.cameraManipulator->setClipPlanes(glm::vec2(0.01f * modelRadius, std::max(50.0f * modelRadius, modelRadius * 2.0f)));
  nvgui::SetHomeCamera(m_info.cameraManipulator->getCamera());
}

void LodClusters::onResize(VkCommandBuffer, const VkExtent2D& size)
{
  m_windowSize = size;
  m_resources.initFramebuffer(m_windowSize, m_tweak.supersample);
  updateImguiImage();
  if(m_renderer)
  {
    m_renderer->updatedFrameBuffer(m_resources, *m_renderScene);
    m_rendererFboChangeID = m_resources.m_fboChangeID;
  }
}

void LodClusters::updateImguiImage()
{
  if(m_imguiTexture)
  {
    ImGui_ImplVulkan_RemoveTexture(m_imguiTexture);
    m_imguiTexture = nullptr;
  }

  VkImageView imageView = m_resources.m_frameBuffer.useResolved ? m_resources.m_frameBuffer.imgColorResolved.descriptor.imageView :
                                                                  m_resources.m_frameBuffer.imgColor.descriptor.imageView;
  if(imageView)
  {
    m_imguiTexture = ImGui_ImplVulkan_AddTexture(m_imguiSampler, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }
}

void LodClusters::onFileDrop(const std::filesystem::path& filePath)
{
  if(filePath.empty())
    return;

  m_sceneFilePathDropLast = filePath;
  m_sceneFilePathDropNew  = filePath;
  deinitRenderer();
  initScene(filePath, false);
}

void LodClusters::handleChanges()
{
  if(m_sceneLoading)
    return;

  bool needRenderer = false;
  if(memcmp(&m_sceneConfig, &m_sceneConfigLast, sizeof(m_sceneConfig)) != 0 && m_scene)
  {
    deinitRenderer();
    initScene(m_sceneFilePath, true);
    needRenderer = false;
  }

  if(memcmp(&m_tweak, &m_tweakLast, sizeof(m_tweak)) != 0)
  {
    if(m_tweak.supersample != m_tweakLast.supersample)
    {
      m_resources.initFramebuffer(m_windowSize, m_tweak.supersample);
      updateImguiImage();
      if(m_renderer)
        m_renderer->updatedFrameBuffer(m_resources, *m_renderScene);
    }
    if(m_tweak.clusterConfig != m_tweakLast.clusterConfig)
    {
      setFromClusterConfig(m_sceneConfig, m_tweak.clusterConfig);
    }
  }

  if(memcmp(&m_rendererConfig, &m_rendererConfigLast, sizeof(m_rendererConfig)) != 0)
  {
    needRenderer = true;
  }

  if(needRenderer)
  {
    initRenderer();
  }

  m_tweakLast          = m_tweak;
  m_sceneConfigLast    = m_sceneConfig;
  m_rendererConfigLast = m_rendererConfig;
}

void LodClusters::onRender(VkCommandBuffer cmd)
{
  double time = m_clock.getSeconds();
  static double lastTime = 0.0;
  float deltaTime = static_cast<float>(time - lastTime);
  lastTime = time;

  m_resources.beginFrame(m_app->getFrameCycleIndex());
  m_frameConfig.windowSize = m_windowSize;

  if(m_renderer)
  {
    if(m_rendererFboChangeID != m_resources.m_fboChangeID)
    {
      m_renderer->updatedFrameBuffer(m_resources, *m_renderScene);
      m_rendererFboChangeID = m_resources.m_fboChangeID;
    }

    shaderio::FrameConstants& frameConstants = m_frameConfig.frameConstants;
    frameConstants.viewProjMatrixPrev = frameConstants.viewProjMatrix;
    if(m_frames)
      m_frameConfig.frameConstantsLast = frameConstants;

    uint32_t renderWidth  = m_resources.m_frameBuffer.renderSize.width;
    uint32_t renderHeight = m_resources.m_frameBuffer.renderSize.height;
    uint32_t targetWidth  = m_resources.m_frameBuffer.targetSize.width;
    uint32_t targetHeight = m_resources.m_frameBuffer.targetSize.height;

    frameConstants.facetShading = m_tweak.facetShading ? 1 : 0;
    frameConstants.visualize    = m_frameConfig.visualize;
    frameConstants.frame        = m_frames;
    frameConstants.deltaTime    = deltaTime;
    frameConstants.time += deltaTime;
    frameConstants.bgColor      = m_resources.m_bgColor;
    frameConstants.viewport     = glm::ivec2(renderWidth, renderHeight);
    frameConstants.viewportf    = glm::vec2(renderWidth, renderHeight);
    frameConstants.nearPlane    = m_info.cameraManipulator->getClipPlanes().x;
    frameConstants.farPlane     = m_info.cameraManipulator->getClipPlanes().y;
    frameConstants.wUpDir       = m_info.cameraManipulator->getUp();
    frameConstants.fov          = glm::radians(m_info.cameraManipulator->getFov());

    glm::mat4 projection = glm::perspectiveRH_ZO(frameConstants.fov, float(targetWidth) / float(targetHeight),
                                                 frameConstants.farPlane, frameConstants.nearPlane);
    projection[1][1] *= -1;
    glm::mat4 view  = m_info.cameraManipulator->getViewMatrix();
    glm::mat4 viewI = glm::inverse(view);

    frameConstants.viewProjMatrix  = projection * view;
    frameConstants.viewProjMatrixI = glm::inverse(frameConstants.viewProjMatrix);
    frameConstants.viewMatrix      = view;
    frameConstants.viewMatrixI     = viewI;
    frameConstants.projMatrix      = projection;
    frameConstants.projMatrixI     = glm::inverse(projection);

    glm::mat4 viewNoTrans = view;
    viewNoTrans[3]        = {0.0f, 0.0f, 0.0f, 1.0f};
    frameConstants.skyProjMatrixI = glm::inverse(projection * viewNoTrans);

    glm::vec4 hPos   = projection * glm::vec4(1.0f, 1.0f, -frameConstants.farPlane, 1.0f);
    glm::vec2 hCoord = glm::vec2(hPos.x / hPos.w, hPos.y / hPos.w);
    glm::vec2 dim    = glm::abs(hCoord);
    frameConstants.viewPixelSize = dim * (glm::vec2(float(renderWidth), float(renderHeight)) * 0.5f) * frameConstants.farPlane;
    frameConstants.viewClipSize  = dim * frameConstants.farPlane;
    frameConstants.viewPos       = frameConstants.viewMatrixI[3];
    frameConstants.viewDir       = -viewI[2];
    frameConstants.viewPlane     = frameConstants.viewDir;
    frameConstants.viewPlane.w   = -glm::dot(glm::vec3(frameConstants.viewPos), glm::vec3(frameConstants.viewDir));
    frameConstants.wLightPos     = frameConstants.viewMatrixI[3];

    if(!m_frames)
      m_frameConfig.frameConstantsLast = frameConstants;
    m_frameConfig.traversalViewMatrix = frameConstants.viewMatrix;

    m_renderer->render(cmd, m_resources, *m_renderScene, m_frameConfig, m_profilerGpuTimer);
  }
  else
  {
    m_resources.emptyFrame(cmd, m_frameConfig, m_profilerGpuTimer);
  }

  m_resources.postProcessFrame(cmd, m_frameConfig, m_profilerGpuTimer);
  m_resources.endFrame();

  VkSemaphoreSubmitInfo semSubmit = m_resources.m_queueStates.primary.advanceSignalSubmit(VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT);
  m_app->addSignalSemaphore(semSubmit);
  while(!m_resources.m_queueStates.primary.m_pendingWaits.empty())
  {
    m_app->addWaitSemaphore(m_resources.m_queueStates.primary.m_pendingWaits.back());
    m_resources.m_queueStates.primary.m_pendingWaits.pop_back();
  }

  m_frames++;
}

void LodClusters::onUIRender()
{
  if(m_sceneLoading)
  {
    ImGui::OpenPopup("Loading");
    if(ImGui::BeginPopupModal("Loading", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
      ImGui::TextDisabled("Please wait ...");
      ImGui::ProgressBar(float(m_sceneProgress) / 100.0f, ImVec2(260, 0), "Loading Scene");
      ImGui::EndPopup();
    }
  }

  namespace PE = nvgui::PropertyEditor;
  if(ImGui::Begin("Settings"))
  {
    PE::begin("settings", ImGuiTableFlags_Resizable);
    PE::entry("Super Resolution", [&]() { return m_ui.enumCombobox(GUI_SUPERSAMPLE, "sampling", &m_tweak.supersample); });
    PE::entry("Visualize", [&]() { return m_ui.enumCombobox(GUI_VISUALIZE, "visualize", &m_frameConfig.visualize); });
    PE::InputFloat("LoD pixel error", &m_frameConfig.lodPixelError, 0.25f, 0.25f, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
    PE::InputIntClamped("Persistent traversal threads", (int*)&m_frameConfig.traversalPersistentThreads, 32, 256 * 1024, 1, 1,
                        ImGuiInputTextFlags_EnterReturnsTrue);
    PE::Checkbox("Facet shading", &m_tweak.facetShading);
    PE::Checkbox("Render stats", &m_rendererConfig.useRenderStats);
    PE::Checkbox("Use EXT mesh shader", &m_rendererConfig.useEXTmeshShader);
    PE::entry("Cluster config", [&]() { return m_ui.enumCombobox(GUI_MESHLET, "cluster", &m_tweak.clusterConfig); });
    PE::InputInt("Cluster group size", (int*)&m_sceneConfig.clusterGroupSize);
    PE::end();
  }
  ImGui::End();

  if(ImGui::Begin("Statistics"))
  {
    if(m_scene)
    {
      ImGui::Text("Triangles: %s", formatMetric(m_scene->m_hiTrianglesCount).c_str());
      ImGui::Text("Clusters: %s", formatMetric(m_scene->m_hiClustersCount).c_str());
      ImGui::Text("Instances: %s", formatMetric(m_scene->m_instances.size()).c_str());
      ImGui::Text("LOD levels max: %u", m_scene->m_maxLodLevelsCount);
    }
    if(m_renderer)
    {
      ImGui::Separator();
      ImGui::Text("Max traversal tasks: %u", m_renderer->getMaxTraversalTasks());
      ImGui::Text("Max render clusters: %u", m_renderer->getMaxRenderClusters());
    }
  }
  ImGui::End();

  if(ImGui::Begin("Misc Settings"))
  {
    nvgui::CameraWidget(m_info.cameraManipulator, false);
    ImGui::Text("Sun & Sky");
    nvgui::skySimpleParametersUI(m_frameConfig.frameConstants.skyParams, "sky", ImGuiTableFlags_Resizable);
  }
  ImGui::End();

  handleChanges();

  if(ImGui::Begin("Viewport"))
  {
    if(m_imguiTexture)
    {
      ImGui::Image((ImTextureID)m_imguiTexture, ImGui::GetContentRegionAvail());
    }
  }
  ImGui::End();
}

void LodClusters::onUIMenu()
{
  bool doOpenFile = ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_O);
  bool doCloseApp = ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Q);

  if(ImGui::BeginMenu("File"))
  {
    if(ImGui::MenuItem("Open", "Ctrl+O"))
      doOpenFile = true;
    if(ImGui::MenuItem("Exit", "Ctrl+Q"))
      doCloseApp = true;
    ImGui::EndMenu();
  }

  if(doOpenFile)
  {
    std::filesystem::path filePath =
        nvgui::windowOpenFileDialog(m_app->getWindowHandle(), "Load model", "glTF|*.gltf;*.glb");
    if(!filePath.empty())
      onFileDrop(filePath);
  }

  if(doCloseApp)
    m_app->close();
}

void LodClusters::parameterSequenceCallback(const nvutils::ParameterSequencer::State& state)
{
  nvutils::Logger::getInstance().log(nvutils::Logger::eSTATS, "Sequence %u %s\n", state.index,
                                     state.description.c_str());
}

const LodClusters::ClusterInfo LodClusters::s_clusterInfos[NUM_CLUSTER_CONFIGS] = {
    {32, 32, CLUSTER_32T_32V},
    {64, 64, CLUSTER_64T_64V},
    {128, 128, CLUSTER_128T_128V},
    {256, 256, CLUSTER_256T_256V},
};

LodClusters::ClusterConfig LodClusters::findSceneClusterConfig(const SceneConfig& sceneConfig) const
{
  for(uint32_t i = 0; i < NUM_CLUSTER_CONFIGS; i++)
  {
    if(sceneConfig.clusterTriangles <= s_clusterInfos[i].tris && sceneConfig.clusterVertices <= s_clusterInfos[i].verts)
      return s_clusterInfos[i].cfg;
  }
  return CLUSTER_256T_256V;
}

void LodClusters::setFromClusterConfig(SceneConfig& sceneConfig, ClusterConfig clusterConfig)
{
  for(uint32_t i = 0; i < NUM_CLUSTER_CONFIGS; i++)
  {
    if(s_clusterInfos[i].cfg == clusterConfig)
    {
      sceneConfig.clusterTriangles = s_clusterInfos[i].tris;
      sceneConfig.clusterVertices  = s_clusterInfos[i].verts;
      return;
    }
  }
}

}  // namespace lodclusters
