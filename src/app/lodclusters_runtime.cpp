#include <volk.h>
#include <nvgui/camera.hpp>
#include "lodclusters.hpp"

namespace lodclusters {

// Adaptive compute-raster routing feedback.

void LodClusters::resetSwRasterFeedback()
{
  m_swRasterFeedback.initialized            = false;
  m_swRasterFeedback.lastBaseExtent         = m_frameConfig.swRasterThreshold;
  m_swRasterFeedback.lastBaseDensity        = m_frameConfig.swRasterTriangleDensityThreshold;
  m_swRasterFeedback.effectiveExtent        = m_frameConfig.swRasterThreshold;
  m_swRasterFeedback.effectiveDensity       = m_frameConfig.swRasterTriangleDensityThreshold;
  m_swRasterFeedback.emaSwClusterShare      = 0.0f;
  m_swRasterFeedback.emaSwTriangleShare     = 0.0f;
  m_swRasterFeedback.emaSwTrianglesPerCluster = 0.0f;

  m_frameConfig.swRasterThresholdEffective = m_frameConfig.swRasterThreshold;
  m_frameConfig.swRasterTriangleDensityThresholdEffective = m_frameConfig.swRasterTriangleDensityThreshold;
}

void LodClusters::updateSwRasterFeedback()
{
  const float baseExtent  = std::max(m_frameConfig.swRasterThreshold, 1.0f);
  const float baseDensity = std::max(m_frameConfig.swRasterTriangleDensityThreshold, 0.01f);

  bool feedbackActive = m_renderer && m_rendererConfig.useComputeRaster && m_rendererConfig.useAdaptiveRasterRouting
                        && m_frameConfig.swRasterFeedbackEnabled;

  if(!feedbackActive)
  {
    resetSwRasterFeedback();
    return;
  }

  if(!m_swRasterFeedback.initialized || m_swRasterFeedback.lastBaseExtent != baseExtent
     || m_swRasterFeedback.lastBaseDensity != baseDensity)
  {
    resetSwRasterFeedback();
    m_swRasterFeedback.initialized = true;
  }

  shaderio::Readback readback;
  m_resources.getReadbackData(readback);

  const uint64_t totalClusters  = uint64_t(readback.numRenderedClusters) + uint64_t(readback.numRenderedClustersSW);
  const uint64_t totalTriangles = uint64_t(readback.numRenderedTriangles) + uint64_t(readback.numRenderedTrianglesSW);

  if(totalClusters == 0 || totalTriangles == 0)
  {
    m_frameConfig.swRasterThresholdEffective = m_swRasterFeedback.effectiveExtent;
    m_frameConfig.swRasterTriangleDensityThresholdEffective = m_swRasterFeedback.effectiveDensity;
    return;
  }

  const float alpha = 0.2f;
  const float swClusterShare = float(readback.numRenderedClustersSW) / float(totalClusters);
  const float swTriangleShare = float(readback.numRenderedTrianglesSW) / float(totalTriangles);
  const float swTrianglesPerCluster =
      readback.numRenderedClustersSW ? float(readback.numRenderedTrianglesSW) / float(readback.numRenderedClustersSW) : 0.0f;

  m_swRasterFeedback.emaSwClusterShare =
      m_swRasterFeedback.emaSwClusterShare * (1.0f - alpha) + swClusterShare * alpha;
  m_swRasterFeedback.emaSwTriangleShare =
      m_swRasterFeedback.emaSwTriangleShare * (1.0f - alpha) + swTriangleShare * alpha;
  m_swRasterFeedback.emaSwTrianglesPerCluster =
      m_swRasterFeedback.emaSwTrianglesPerCluster * (1.0f - alpha) + swTrianglesPerCluster * alpha;

  const bool enoughSamples = totalClusters >= 64;
  if(enoughSamples)
  {
    const float targetTriangleShare = glm::clamp(m_frameConfig.swRasterFeedbackTargetTriangleShare, 0.02f, 0.75f);
    const float deadzone            = std::max(0.015f, targetTriangleShare * 0.15f);
    const float shareError          = m_swRasterFeedback.emaSwTriangleShare - targetTriangleShare;
    const float stepExtent          = 0.35f;
    const float stepDensity         = 0.04f;
    const float highTrianglesPerCluster = std::min(float(m_sceneConfig.clusterTriangles) * 0.55f, 96.0f);

    float errorScale = 0.0f;
    if(shareError > deadzone)
    {
      errorScale = glm::clamp((shareError - deadzone) / std::max(1.0f - targetTriangleShare, 0.1f), 0.0f, 1.0f);
      m_swRasterFeedback.effectiveExtent -= stepExtent * (0.35f + errorScale);
      m_swRasterFeedback.effectiveDensity += stepDensity * (0.35f + errorScale);
    }
    else if(shareError < -deadzone)
    {
      errorScale = glm::clamp((-shareError - deadzone) / std::max(targetTriangleShare, 0.1f), 0.0f, 1.0f);
      m_swRasterFeedback.effectiveExtent += stepExtent * (0.35f + errorScale);
      m_swRasterFeedback.effectiveDensity -= stepDensity * (0.35f + errorScale);
    }

    if(m_swRasterFeedback.emaSwTrianglesPerCluster > highTrianglesPerCluster)
    {
      m_swRasterFeedback.effectiveExtent -= stepExtent * 0.5f;
      m_swRasterFeedback.effectiveDensity += stepDensity * 0.5f;
    }
  }

  const float minExtent   = std::max(1.0f, baseExtent * 0.5f);
  const float maxExtent   = std::max(baseExtent * 2.0f, baseExtent + 4.0f);
  const float minDensity  = std::max(0.05f, baseDensity * 0.35f);
  const float maxDensity  = std::max(baseDensity * 3.0f, baseDensity + 0.75f);

  m_swRasterFeedback.effectiveExtent  = glm::clamp(m_swRasterFeedback.effectiveExtent, minExtent, maxExtent);
  m_swRasterFeedback.effectiveDensity = glm::clamp(m_swRasterFeedback.effectiveDensity, minDensity, maxDensity);

  m_frameConfig.swRasterThresholdEffective = m_swRasterFeedback.effectiveExtent;
  m_frameConfig.swRasterTriangleDensityThresholdEffective = m_swRasterFeedback.effectiveDensity;
}

void LodClusters::onPreRender()
{
  updateSwRasterFeedback();
  m_profilerTimeline->frameAdvance();
}

// Runtime configuration reconciliation.

void LodClusters::handleChanges()
{
  if(m_sceneLoading)
    return;

  if(m_sceneFilePathDropLast != m_sceneFilePathDropNew)
  {
    std::filesystem::path newFilePath = m_sceneFilePathDropNew;
    onFileDrop(newFilePath);
  }

  if(!m_resources.m_supportsMeshShaderNV)
  {
    m_rendererConfig.useEXTmeshShader = true;
  }


  if((m_frameConfig.visualize == VISUALIZE_VIS_BUFFER || m_frameConfig.visualize == VISUALIZE_DEPTH_ONLY)
     && m_rendererConfig.useShading)
  {
    m_rendererConfig.useShading = false;
  }
  if(!(m_frameConfig.visualize == VISUALIZE_VIS_BUFFER || m_frameConfig.visualize == VISUALIZE_DEPTH_ONLY)
     && !m_rendererConfig.useShading)
  {
    m_rendererConfig.useShading = true;
  }
  m_rendererConfig.useDepthOnly = m_frameConfig.visualize == VISUALIZE_DEPTH_ONLY;


  if(m_rendererConfig.useComputeRaster
     && (!m_rendererConfig.useSeparateGroups || !m_rendererConfig.useCulling || m_rendererConfig.useShading))
  {
    m_rendererConfig.useComputeRaster = false;
  }
  if(!m_rendererConfig.useComputeRaster)
  {
    m_rendererConfig.useAdaptiveRasterRouting = false;
  }


  bool frameBufferChanged = false;
  if(tweakChanged(m_tweak.supersample))
  {
    m_resources.initFramebuffer(m_windowSize, m_tweak.supersample);
    updateImguiImage();

    frameBufferChanged = true;
  }

  bool shaderChanged = false;
  if(m_reloadShaders)
  {
    shaderChanged   = true;
    m_reloadShaders = false;
  }

  bool sceneChanged = false;
  if(memcmp(&m_sceneConfig, &m_sceneConfigLast, sizeof(m_sceneConfig)))
  {
    sceneChanged = true;

    deinitRenderer();
    initScene(m_sceneFilePath, m_scene->m_cacheSuffix, true);
  }

  if(!m_cameraString.empty() && m_cameraString != m_cameraStringLast)
  {
    applyCameraString();
  }

  bool sceneGridChanged = false;
  if(m_scene)
  {
    if(!m_renderScene)
    {
      sceneGridChanged = true;
    }

    if(!sceneChanged && memcmp(&m_sceneGridConfig, &m_sceneGridConfigLast, sizeof(m_sceneGridConfig)))
    {
      sceneGridChanged = true;

      deinitRenderer();
      m_scene->updateSceneGrid(m_sceneGridConfig);
      updatedSceneGrid();
    }

    bool renderSceneChanged = false;
    bool streamingChanged   = tweakChanged(m_tweak.useStreaming)
                            || (memcmp(&m_streamingConfig, &m_streamingConfigLast, sizeof(m_streamingConfig)));
    if(sceneGridChanged || streamingChanged)
    {
      if(!sceneChanged || !sceneGridChanged)
      {
        deinitRenderer();
      }

      renderSceneChanged = true;
      deinitRenderScene();
      initRenderScene();

      if(streamingChanged)
      {
        m_streamGeometryHistogramMax = 0;
      }
    }
    // 检查场景、着色器或渲染场景是否有变化
    if(sceneChanged || shaderChanged || renderSceneChanged || tweakChanged(m_tweak.renderer) || tweakChanged(m_tweak.supersample)
       || rendererCfgChanged(m_rendererConfig.flipWinding) || rendererCfgChanged(m_rendererConfig.useDebugVisualization)
       || rendererCfgChanged(m_rendererConfig.useCulling) || rendererCfgChanged(m_rendererConfig.forceTwoSided)
       || rendererCfgChanged(m_rendererConfig.useSorting) || rendererCfgChanged(m_rendererConfig.numRenderClusterBits)
       || rendererCfgChanged(m_rendererConfig.numTraversalTaskBits) || rendererCfgChanged(m_rendererConfig.useShading)
       || rendererCfgChanged(m_rendererConfig.useRenderStats)
       || rendererCfgChanged(m_rendererConfig.useSeparateGroups) 
       || rendererCfgChanged(m_rendererConfig.useEXTmeshShader)
       || rendererCfgChanged(m_rendererConfig.useComputeRaster) || rendererCfgChanged(m_rendererConfig.useAdaptiveRasterRouting)
       || rendererCfgChanged(m_rendererConfig.usePrimitiveCulling)
        //|| rendererCfgChanged(m_rendererConfig.useComputeRaster) || rendererCfgChanged(m_rendererConfig.usePrimitiveCulling))
       || rendererCfgChanged(m_rendererConfig.useTwoPassCulling) || rendererCfgChanged(m_rendererConfig.useDepthOnly)
       || rendererCfgChanged(m_rendererConfig.useForcedInvisibleCulling))
    {

      initRenderer(m_tweak.renderer);
    }
    else if(m_renderer && frameBufferChanged)
    {
      m_renderer->updatedFrameBuffer(m_resources, *m_renderScene);
      m_rendererFboChangeID = m_resources.m_fboChangeID;
    }
  }


  bool hadChange = shaderChanged || memcmp(&m_tweakLast, &m_tweak, sizeof(m_tweak))
                   || memcmp(&m_rendererConfigLast, &m_rendererConfig, sizeof(m_rendererConfig))
                   || memcmp(&m_sceneConfigLast, &m_sceneConfig, sizeof(m_sceneConfig))
                   || memcmp(&m_streamingConfigLast, &m_streamingConfig, sizeof(m_streamingConfig))
                   || memcmp(&m_sceneGridConfigLast, &m_sceneGridConfig, sizeof(m_sceneGridConfig));
  m_tweakLast           = m_tweak;
  m_rendererConfigLast  = m_rendererConfig;
  m_streamingConfigLast = m_streamingConfig;
  m_sceneConfigLast     = m_sceneConfig;
  m_sceneGridConfigLast = m_sceneGridConfig;

  if(hadChange)
  {
    m_equalFrames = 0;
    if(m_tweak.autoResetTimers)
    {
      m_info.profilerManager->resetFrameSections(8);
    }
  }
}

void LodClusters::applyCameraString()
{
  nvutils::CameraManipulator::Camera cam = m_info.cameraManipulator->getCamera();
  if(cam.setFromString(m_cameraString))
  {
    m_info.cameraManipulator->setCamera(cam);
    nvgui::SetHomeCamera(m_info.cameraManipulator->getCamera());
  }
  m_cameraStringLast = m_cameraString;
}

// 渲染函数
// 执行场景渲染，包括设置帧常量、更新相机矩阵、执行渲染器渲染等
// 参数: cmd - Vulkan命令缓冲区

// Per-frame camera constants and render dispatch.

void LodClusters::onRender(VkCommandBuffer cmd)
{
  double time = m_clock.getSeconds();
  static double lastTime = 0.0;
  float deltaTime = static_cast<float>(time - lastTime);
  lastTime = time;

  // 开始新帧
  m_resources.beginFrame(m_app->getFrameCycleIndex());

  // 设置窗口大小
  m_frameConfig.windowSize = m_windowSize;

  // 检查渲染器是否存在
  if(m_renderer)
  {
    // 检查帧缓冲区是否有变化
    if(m_rendererFboChangeID != m_resources.m_fboChangeID)
    {
      m_renderer->updatedFrameBuffer(m_resources, *m_renderScene);
      m_rendererFboChangeID = m_resources.m_fboChangeID;
    }


    shaderio::FrameConstants& frameConstants = m_frameConfig.frameConstants;

    // for motion always use last
    frameConstants.viewProjMatrixPrev = frameConstants.viewProjMatrix;

    if(m_frames)
    {
      m_frameConfig.frameConstantsLast = m_frameConfig.frameConstants;
    }

    int supersample = m_tweak.supersample;
    //uint32_t windowWidth = m_resources.m_frameBuffer.windowSize.width;
    //uint32_t windowHeight = m_resources.m_frameBuffer.windowSize.height;
    uint32_t renderWidth  = m_resources.m_frameBuffer.renderSize.width;
    uint32_t renderHeight = m_resources.m_frameBuffer.renderSize.height;

    uint32_t targetWidth  = m_resources.m_frameBuffer.targetSize.width;
    uint32_t targetHeight = m_resources.m_frameBuffer.targetSize.height;
    // 设置帧常量的渲染属性
    frameConstants.facetShading = m_tweak.facetShading ? 1 : 0;
    frameConstants.visualize    = m_frameConfig.visualize;
    frameConstants.frame        = m_frames;
    
    // 更新时间相关字段用于LOD平滑过渡
    static float accumulatedTime = 0.0f;
    accumulatedTime += deltaTime;
    frameConstants.time = accumulatedTime;
    frameConstants.deltaTime = deltaTime;

    {
      frameConstants.visFilterClusterID  = ~0;
      frameConstants.visFilterInstanceID = ~0;
    }

    frameConstants.bgColor   = m_resources.m_bgColor;
    frameConstants.viewport  = glm::ivec2(renderWidth, renderHeight);
    frameConstants.viewportf = glm::vec2(renderWidth, renderHeight);
    //frameConstants.supersample = m_tweak.supersample;
    frameConstants.nearPlane = m_info.cameraManipulator->getClipPlanes().x;
    frameConstants.farPlane  = m_info.cameraManipulator->getClipPlanes().y;
    frameConstants.wUpDir    = m_info.cameraManipulator->getUp();
    frameConstants.fov = glm::radians(m_info.cameraManipulator->getFov());
    //glm::perspectiveRH_ZO(glm::radians(m_info.cameraManipulator->getFov()), float(windowWidth) / float(windowHeight),
    //glm::perspectiveRH_ZO(glm::radians(m_info.cameraManipulator->getFov()), float(targetWidth) / float(targetHeight),
    glm::mat4 projection = glm::perspectiveRH_ZO(frameConstants.fov, float(targetWidth) / float(targetHeight), frameConstants.farPlane, frameConstants.nearPlane);
    projection[1][1] *= -1;
    glm::mat4 view  = m_info.cameraManipulator->getViewMatrix();
    glm::mat4 viewI = glm::inverse(view);

    frameConstants.viewProjMatrix  = projection * view;
    frameConstants.viewProjMatrixI = glm::inverse(frameConstants.viewProjMatrix);
    frameConstants.viewMatrix      = view;
    frameConstants.viewMatrixI     = viewI;
    frameConstants.projMatrix      = projection;
    frameConstants.projMatrixI     = glm::inverse(projection);

    glm::mat4 viewNoTrans         = view;
    viewNoTrans[3]                = {0.0f, 0.0f, 0.0f, 1.0f};
    frameConstants.skyProjMatrixI = glm::inverse(projection * viewNoTrans);

    glm::vec4 hPos   = projection * glm::vec4(1.0f, 1.0f, -frameConstants.farPlane, 1.0f);
    glm::vec2 hCoord = glm::vec2(hPos.x / hPos.w, hPos.y / hPos.w);
    glm::vec2 dim    = glm::abs(hCoord);

    // helper to quickly get footprint of a point at a given distance
    //
    // __.__hPos (far plane is width x height)
    // \ | /
    //  \|/
    //   x camera
    //
    // here: viewPixelSize / point.w = size of point in pixels
    // * 0.5f because renderWidth/renderHeight represents [-1,1] but we need half of frustum
    frameConstants.viewPixelSize = dim * (glm::vec2(float(renderWidth), float(renderHeight)) * 0.5f) * frameConstants.farPlane;
    // here: viewClipSize / point.w = size of point in clip-space units
    // no extra scale as half clip space is 1.0 in extent
    frameConstants.viewClipSize = dim * frameConstants.farPlane;

    frameConstants.viewPos = frameConstants.viewMatrixI[3];  // position of eye in the world
    frameConstants.viewDir = -viewI[2];

    frameConstants.viewPlane   = frameConstants.viewDir;
    frameConstants.viewPlane.w = -glm::dot(glm::vec3(frameConstants.viewPos), glm::vec3(frameConstants.viewDir));

    frameConstants.wLightPos = frameConstants.viewMatrixI[3];  // place light at position of eye in the world

    {
      // hiz
      //m_resources.m_hizUpdate.farInfo.getShaderFactors((float*)&frameConstants.hizSizeFactors);
      //frameConstants.hizSizeMax = m_resources.m_hizUpdate.farInfo.getSizeMax();
      m_resources.m_hizUpdate[0].farInfo.getShaderFactors((float*)&frameConstants.hizSizeFactors);
      frameConstants.hizSizeMax = m_resources.m_hizUpdate[0].farInfo.getSizeMax();
      // 注：在 resources.hpp 中定义了：
      // NVHizVK::Update m_hizUpdate[2];
      // [0] = 前一帧 HiZ
      // [1] = 当前帧 HiZ（用于时间上的平滑过渡）
    }


    if(!m_frames)
    {
      // on first frame replicate last
      m_frameConfig.frameConstantsLast = m_frameConfig.frameConstants;
    }

    if(!m_frameConfig.freezeLoD)
    {
      m_frameConfig.traversalViewMatrix = m_frameConfig.frameConstants.viewMatrix;
    }
    if(!m_frameConfig.freezeCulling)
    {
      m_frameConfig.cullViewProjMatrix     = m_frameConfig.frameConstants.viewProjMatrix;
      m_frameConfig.cullViewProjMatrixLast = m_frameConfig.frameConstantsLast.viewProjMatrix;
    }

    if(m_frames)
    {
      shaderio::FrameConstants frameCurrent = m_frameConfig.frameConstants;

      if(memcmp(&frameCurrent, &m_frameConfig.frameConstantsLast, sizeof(shaderio::FrameConstants)))
        m_equalFrames = 0;
      else
        m_equalFrames++;
    }

    m_renderer->render(cmd, m_resources, *m_renderScene, m_frameConfig, m_profilerGpuTimer);
  }
  else
  {
    m_resources.emptyFrame(cmd, m_frameConfig, m_profilerGpuTimer);
  }

  {
    m_resources.postProcessFrame(cmd, m_frameConfig, m_profilerGpuTimer);
  }

  m_resources.endFrame();

  // signal new semaphore state with this command buffer's submit
  VkSemaphoreSubmitInfo semSubmit = m_resources.m_queueStates.primary.advanceSignalSubmit(VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT);
  m_app->addSignalSemaphore(semSubmit);
  // but also enqueue waits if there are any
  while(!m_resources.m_queueStates.primary.m_pendingWaits.empty())
  {
    m_app->addWaitSemaphore(m_resources.m_queueStates.primary.m_pendingWaits.back());
    m_resources.m_queueStates.primary.m_pendingWaits.pop_back();
  }

  m_lastTime = time;
  m_frames++;
}

}  // namespace lodclusters
