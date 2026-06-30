//==============================================================================
// 鏂囦欢锛歴rc/app/lodclusters_lifecycle.cpp
// 妯″潡瀹氫綅锛氬簲鐢ㄦ寕杞姐€侀噴鏀俱€佺獥鍙ｅ昂瀵稿彉鍖栥€佹覆鏌撳櫒閲嶅缓鍜?ImGui 绾圭悊妗ユ帴鐨勭敓鍛藉懆鏈熷疄鐜般€?// 鏁版嵁娴侊細杈撳叆鏄?nvapp 鍥炶皟鍜岀獥鍙ｅぇ灏忥紱杈撳嚭鏄凡鍖归厤褰撳墠 甯х紦鍐?鐨勮祫婧愩€佹弿杩扮鍜?renderer 瀹炰緥銆?// 鏂规硶璇存槑锛氱敓鍛藉懆鏈熷嚱鏁版妸 Vulkan 瀵硅薄鐨勫垱寤?閿€姣佹嫇鎵戝浐瀹氫笅鏉ワ紝閬垮厤鍥惧儚銆佹弿杩扮鍜?绠＄嚎 涔嬮棿鍑虹幇鎮瀭寮曠敤銆?// 姝ｇ‘鎬х害鏉燂細閲婃斁鍓嶉渶瑕佺瓑寰?GPU 绌洪棽锛涘抚缂撳啿 鍙樺寲蹇呴』鍒锋柊 ImGui 鍥惧儚鍜?renderer 鎻忚堪绗︼紱renderer 閿€姣佹棭浜?Resources 閿€姣併€?// 娉ㄩ噴椋庢牸锛氫娇鐢ㄤ腑鏂囪В閲?CPU 渚ц涔夛紱淇濈暀蹇呰鐨?API銆佺被鍨嬪悕鍜屾暟瀛︾缉鍐欎互渚挎绱€?//==============================================================================
// 渚濊禆璇存槑锛氬紩鍏ユ湰缂栬瘧鍗曞厓闇€瑕佺殑澶栭儴搴撱€侀」鐩ā鍧楀拰鍏变韩鐫€鑹插櫒甯冨眬銆?// 渚濊禆椤哄簭閫氬父鍙嶆槧鎶借薄灞傛锛氬厛澶栭儴搴擄紝鍐嶉」鐩ā鍧楋紝鏈€鍚庝笌 GPU 鍏变韩鐨勬帴鍙ｅ畾涔夈€?#include <volk.h>
#include <volk.h>
#include <fmt/format.h>
#include <nvutils/file_operations.hpp>
#include "lodclusters.hpp"


// 鍛藉悕绌洪棿璇存槑锛氶檺鍒剁鍙峰彲瑙佽寖鍥达紝骞惰〃鏄庤繖浜涚被鍨嬪拰鍑芥暟灞炰簬鍚屼竴鍔熻兘鍩熴€?// 璇ヨ竟鐣屾湁鍔╀簬鍖哄垎搴旂敤灞傘€佹覆鏌撳眰銆佸満鏅眰鍜岀畻娉曞眰鐨勮亴璐ｃ€?namespace lodclusters {
namespace lodclusters {


// 鍑芥暟锛歀odClusters::onResize銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?void LodClusters::onResize(VkCommandBuffer cmd, const VkExtent2D& size)
void LodClusters::onResize(VkCommandBuffer cmd, const VkExtent2D& size)
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


// 鍑芥暟锛歀odClusters::updateImguiImage銆傛牴鎹渶鏂扮姸鎬佸埛鏂扮紦瀛樻暟鎹€丟PU 鍦板潃銆佹弿杩扮鎴栫粺璁′俊鎭€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氭洿鏂板嚱鏁拌礋璐ｆ妸鈥滄棫鐘舵€佲€濇帹杩涘埌鈥滃綋鍓嶇姸鎬佲€濓紝鍥犳瑕侀伩鍏嶉儴鍒嗘洿鏂伴€犳垚 CPU/GPU 瑙嗗浘涓嶄竴鑷淬€?void LodClusters::updateImguiImage()
void LodClusters::updateImguiImage()
{
  if(m_imguiTexture)
  {

    ImGui_ImplVulkan_RemoveTexture(m_imguiTexture);
    m_imguiTexture = nullptr;
  }

  VkImageView imageView = m_resources.m_frameBuffer.useResolved ? m_resources.m_frameBuffer.imgColorResolved.descriptor.imageView :
                                                                  m_resources.m_frameBuffer.imgColor.descriptor.imageView;


  assert(imageView);


  m_imguiTexture = ImGui_ImplVulkan_AddTexture(m_imguiSampler, imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}


// 鍑芥暟锛歀odClusters::deinitRenderer銆傞噴鏀炬垨鍥炴敹鍓嶉潰鍒濆鍖栫殑璧勬簮锛屼繚鎸佺敓鍛藉懆鏈熸垚瀵圭鐞嗐€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氶噴鏀鹃『搴忚閬靛畧璧勬簮渚濊禆鍏崇郴锛岄伩鍏?GPU 浠嶅彲鑳借闂殑瀵硅薄琚彁鍓嶉攢姣併€?void LodClusters::deinitRenderer()
void LodClusters::deinitRenderer()
{
  NVVK_CHECK(vkDeviceWaitIdle(m_app->getDevice()));

  if(m_renderer)
  {

    m_renderer->deinit(m_resources);
    m_renderer = nullptr;
  }
}


// 鍑芥暟锛歀odClusters::initRenderer銆傚垵濮嬪寲鏈ā鍧楁墍闇€鐘舵€併€佽祫婧愭垨 GPU 渚х粦瀹氥€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氬垵濮嬪寲杩囩▼寤虹珛鍚庣画闃舵鍋囧畾瀛樺湪鐨勪笉鍙橀噺锛屼緥濡傚彞鏌勬湁鏁堛€佺紦鍐插ぇ灏忚冻澶熴€佹弿杩扮宸茬粦瀹氥€?void LodClusters::initRenderer(RendererType rtype)
void LodClusters::initRenderer(RendererType rtype)
{

  LOGI("Initializing renderer and compiling shaders\n");

  deinitRenderer();
  if(!m_renderScene)
    return;


  printf("init renderer %d\n", rtype);

  if(m_renderScene->useStreaming)
  {
    if(!m_renderScene->sceneStreaming.reloadShaders())
    {

      LOGE("RenderScene shaders failed\n");
      return;
    }
  }

  switch(rtype)
  {
    case RENDERER_RASTER_CLUSTERS_LOD:

      m_renderer = makeRendererRasterClustersLod();
      break;
  }

  if(m_renderer && !m_renderer->init(m_resources, *m_renderScene, m_rendererConfig))
  {
    m_renderer = nullptr;

    LOGE("Renderer init failed\n");
  }

  m_rendererFboChangeID = m_resources.m_fboChangeID;
}


// 鍑芥暟锛歀odClusters::onAttach銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?void LodClusters::onAttach(nvapp::Application* app)
void LodClusters::onAttach(nvapp::Application* app)
{
  m_app = app;


  m_tweak.supersample = std::max(1, m_tweak.supersample);

  m_info.cameraManipulator->setMode(nvutils::CameraManipulator::Fly);
  m_renderer = nullptr;

  if(m_resources.m_supportsSmBuiltinsNV)
  {
    VkPhysicalDeviceProperties2 physicalProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    VkPhysicalDeviceShaderSMBuiltinsPropertiesNV smProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_PROPERTIES_NV};
    physicalProperties.pNext = &smProperties;
    vkGetPhysicalDeviceProperties2(app->getPhysicalDevice(), &physicalProperties);


    if(smProperties.shaderSMCount * smProperties.shaderWarpsPerSM > 4096)
      m_frameConfig.traversalPersistentThreads = smProperties.shaderSMCount * smProperties.shaderWarpsPerSM * 2;
    else if(smProperties.shaderSMCount * smProperties.shaderWarpsPerSM > 2048 + 1024)
      m_frameConfig.traversalPersistentThreads = smProperties.shaderSMCount * smProperties.shaderWarpsPerSM * 4;
    else
      m_frameConfig.traversalPersistentThreads = smProperties.shaderSMCount * smProperties.shaderWarpsPerSM * 8;
  }

  {

    m_ui.enumAdd(GUI_RENDERER, RENDERER_RASTER_CLUSTERS_LOD, "Rasterization");

    {
      for(uint32_t i = 0; i < NUM_CLUSTER_CONFIGS; i++)
      {
        std::string enumStr = fmt::format("{}T_{}V", s_clusterInfos[i].tris, s_clusterInfos[i].verts);
        m_ui.enumAdd(GUI_MESHLET, s_clusterInfos[i].cfg, enumStr.c_str());
      }
    }


    m_ui.enumAdd(GUI_SUPERSAMPLE, 1, "none");

    m_ui.enumAdd(GUI_SUPERSAMPLE, 2, "4x");

    m_ui.enumAdd(GUI_SUPERSAMPLE, 720, "720p");

    m_ui.enumAdd(GUI_SUPERSAMPLE, 1080, "1080p");

    m_ui.enumAdd(GUI_SUPERSAMPLE, 1440, "1440p");

    m_ui.enumAdd(GUI_SUPERSAMPLE, 2160, "2160p");

    m_ui.enumAdd(GUI_SUPERSAMPLE, 1024, "1024 sq");

    m_ui.enumAdd(GUI_SUPERSAMPLE, 2048, "2048 sq");

    m_ui.enumAdd(GUI_SUPERSAMPLE, 4096, "4096 sq");

    m_ui.enumAdd(GUI_VISUALIZE, VISUALIZE_MATERIAL, "material");

    m_ui.enumAdd(GUI_VISUALIZE, VISUALIZE_GREY, "grey");

    m_ui.enumAdd(GUI_VISUALIZE, VISUALIZE_VIS_BUFFER, "visibility buffer");

    m_ui.enumAdd(GUI_VISUALIZE, VISUALIZE_CLUSTER, "clusters");

    m_ui.enumAdd(GUI_VISUALIZE, VISUALIZE_GROUP, "cluster groups");

    m_ui.enumAdd(GUI_VISUALIZE, VISUALIZE_LOD, "lod levels");

    m_ui.enumAdd(GUI_VISUALIZE, VISUALIZE_TRIANGLE, "triangles");

    m_ui.enumAdd(GUI_VISUALIZE, VISUALIZE_SEMANTIC_LOD, "semantic lod policy");

  }


  m_profilerGpuTimer.init(m_profilerTimeline, app->getDevice(), app->getPhysicalDevice(), app->getQueue(0).familyIndex, true);
  m_resources.init(app->getDevice(), app->getPhysicalDevice(), app->getInstance(), app->getQueue(0), app->getQueue(1));

  {
    NVVK_CHECK(m_resources.m_samplerPool.acquireSampler(m_imguiSampler));

    NVVK_DBG_NAME(m_imguiSampler);
  }

  m_resources.initFramebuffer({128, 128}, m_tweak.supersample);

  updateImguiImage();


  setFromClusterConfig(m_sceneConfig, m_tweak.clusterConfig);

  if(!m_resources.m_supportsMeshShaderNV)
  {
    m_rendererConfig.useEXTmeshShader = true;
  }


  if(m_sceneFilePathDropNew.empty())
  {

    const std::filesystem::path              exeDirectoryPath   = nvutils::getExecutablePath().parent_path();
    const std::vector<std::filesystem::path> defaultSearchPaths = {

        std::filesystem::absolute(exeDirectoryPath / TARGET_EXE_TO_DOWNLOAD_DIRECTORY),

        std::filesystem::absolute(exeDirectoryPath / "resources"),
    };


    m_sceneFilePathDefault = m_sceneFilePathDropNew = nvutils::findFile("bunny_v2/bunny.gltf", defaultSearchPaths);


    m_sceneGridConfig.uniqueGeometriesForCopies = false;

    if(m_sceneGridConfig.numCopies == 1)
    {
      if(m_resources.getDeviceLocalHeapSize() >= 8ull * 1024 * 1024 * 1024)
      {
        m_sceneGridConfig.numCopies = 1;
      }
      else
      {
        m_sceneGridConfig.numCopies =1;
      }
    }
  }

  m_cameraStringCommandLine = m_cameraString;

  std::filesystem::path newFileDrop = m_sceneFilePathDropNew;

  onFileDrop(newFileDrop);

  m_tweakLast          = m_tweak;
  m_sceneConfigLast    = m_sceneConfig;
  m_sceneConfigEdit    = m_sceneConfig;
  m_rendererConfigLast = m_rendererConfig;
}


// 鍑芥暟锛歀odClusters::onDetach銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?void LodClusters::onDetach()
void LodClusters::onDetach()
{
  NVVK_CHECK(vkDeviceWaitIdle(m_app->getDevice()));


  deinitRenderer();

  deinitScene();


  m_resources.m_samplerPool.releaseSampler(m_imguiSampler);

  ImGui_ImplVulkan_RemoveTexture(m_imguiTexture);


  m_resources.deinit();


  m_profilerGpuTimer.deinit();
}


// 鍑芥暟锛歀odClusters::parameterSequenceCallback銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?void LodClusters::parameterSequenceCallback(const nvutils::ParameterSequencer::State& state)
void LodClusters::parameterSequenceCallback(const nvutils::ParameterSequencer::State& state)
{
  std::string message;
  message += fmt::format("MemoryReport {} \"{}\" = {{ \n", state.index, state.description);
  if(m_renderer)
  {

    Renderer::ResourceUsageInfo resourceActual   = m_renderer->getResourceUsage(false);

    Renderer::ResourceUsageInfo resourceReserved = m_renderer->getResourceUsage(true);
    message += fmt::format("Memory; Actual; Reserved;\n");
    message += fmt::format("Geometry; {}; {};\n", resourceActual.geometryMemBytes, resourceReserved.geometryMemBytes);
    if(m_renderScene->useStreaming)
    {
      StreamingStats stats;

      m_renderScene->sceneStreaming.getStats(stats);
      message += fmt::format("Resident; Actual; Reserved;\n");
      message += fmt::format("Groups; {}; {};\n", stats.residentGroups, stats.maxGroups);
      message += fmt::format("Clusters; {}; {};\n", stats.residentClusters, stats.maxClusters);
    }

    shaderio::Readback readback;

    m_resources.getReadbackData(readback);
    message += fmt::format("Traversal; Actual; Reserved;\n");
    message += fmt::format("Traversal Tasks; {}; {};\n", readback.numTraversalTasks, m_renderer->getMaxTraversalTasks());
    message += fmt::format("Traversal Clusters; {}; {};\n", readback.numRenderClusters, m_renderer->getMaxRenderClusters());

  }
  message += fmt::format("}}\n");

  nvutils::Logger::getInstance().log(nvutils::Logger::eSTATS, "%s", message.c_str());

  if(m_sequenceScreenshotMode != SCREENSHOT_OFF)
  {
    ScreenshotMode screenshotMode = m_sequenceScreenshotMode;
    if(m_app->isHeadless())
    {
      screenshotMode = SCREENSHOT_VIEWPORT;
    }

    std::string filename = fmt::format("screenshot_{}_{}.jpg", state.index, state.description);
    if(screenshotMode == SCREENSHOT_WINDOW)
    {
      m_app->saveScreenShot(std::filesystem::path(filename), 100);
    }
    else if(screenshotMode == SCREENSHOT_VIEWPORT)
    {
      m_app->saveImageToFile(m_resources.m_frameBuffer.useResolved ? m_resources.m_frameBuffer.imgColorResolved.image :
                                                                     m_resources.m_frameBuffer.imgColor.image,
                             m_resources.m_frameBuffer.windowSize, filename, 100, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
  }
}

}
