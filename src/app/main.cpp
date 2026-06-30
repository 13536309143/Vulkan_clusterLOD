//==============================================================================
// 鏂囦欢锛歴rc/app/main.cpp
// 妯″潡瀹氫綅锛氬簲鐢ㄧ▼搴忓叆鍙ｏ紝璐熻矗鎶婄獥鍙ｇ郴缁熴€佸弬鏁扮郴缁熴€乂ulkan 涓婁笅鏂囥€佹€ц兘鍒嗘瀽鍣ㄥ拰 LodClusters 搴旂敤鍏冪礌杩炴帴鎴愬畬鏁磋繍琛屼綋銆?// 鏁版嵁娴侊細杈撳叆鏉ヨ嚜鍛戒护琛屻€侀厤缃枃浠跺拰杩愯鐜璁惧鑳藉姏锛涜緭鍑烘槸宸茬粡鍒濆鍖栫殑 nvapp 搴旂敤銆侀€昏緫璁惧銆侀槦鍒椼€佺晫闈㈠竷灞€鍜屼富寰幆銆?// 鏂规硶璇存槑锛氳鏂囦欢浣撶幇鈥滅粍鍚堟牴鈥濇ā寮忥細绠楁硶妯″潡涓嶅湪鍏ュ彛澶勫睍寮€锛岃€屾槸閫氳繃鏄庣‘鐨勫垵濮嬪寲椤哄簭寤虹珛渚濊禆鍥撅紝閬垮厤璺ㄥ眰璧勬簮鐢熷懡鍛ㄦ湡澶遍厤銆?// 姝ｇ‘鎬х害鏉燂細Vulkan 鐗规€ч摼蹇呴』鍦ㄥ垱寤鸿澶囧墠瀹屾垚锛沺rocessing-only 妯″紡蹇呴』鍦ㄦ棤绐楀彛璺緞涓嬫彁鍓嶇粨鏉燂紱娓呯悊椤哄簭搴斾笌鍒濆鍖栭『搴忕浉鍙嶃€?// 娉ㄩ噴椋庢牸锛氫娇鐢ㄤ腑鏂囪В閲?CPU 渚ц涔夛紱淇濈暀蹇呰鐨?API銆佺被鍨嬪悕鍜屾暟瀛︾缉鍐欎互渚挎绱€?//==============================================================================
#ifndef NDEBUG


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define VMA_LEAK_LOG_FORMAT(format, ...)                                                                               \
#define VMA_LEAK_LOG_FORMAT(format, ...)                                                                               \
  do                                                                                                                   \
  {                                                                                                                    \
    fprintf(stderr, (format), __VA_ARGS__);                                                                            \
    fprintf(stderr, "\n");                                                                                             \
  } while(false)
#endif


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define VMA_IMPLEMENTATION
#define VMA_IMPLEMENTATION

#if __INTELLISENSE__
#undef VK_NO_PROTOTYPES
#endif


// 渚濊禆璇存槑锛氬紩鍏ユ湰缂栬瘧鍗曞厓闇€瑕佺殑澶栭儴搴撱€侀」鐩ā鍧楀拰鍏变韩鐫€鑹插櫒甯冨眬銆?// 渚濊禆椤哄簭閫氬父鍙嶆槧鎶借薄灞傛锛氬厛澶栭儴搴擄紝鍐嶉」鐩ā鍧楋紝鏈€鍚庝笌 GPU 鍏变韩鐨勬帴鍙ｅ畾涔夈€?#include <volk.h>
#include <volk.h>
#include <imgui/imgui.h>
#include <nvvk/validation_settings.hpp>
#include <nvapp/elem_logger.hpp>
#include <nvapp/elem_profiler.hpp>
#include <nvapp/elem_camera.hpp>
#include <nvapp/elem_default_menu.hpp>
#include <nvapp/elem_default_title.hpp>
#include <nvapp/elem_sequencer.hpp>
#include <nvutils/parameter_parser.hpp>

#include "lodclusters.hpp"

using namespace lodclusters;


// 鍑芥暟锛歮ain銆備綔涓虹▼搴忓叆鍙ｏ紝涓茶仈鍒濆鍖栥€佽繍琛屽拰娓呯悊娴佺▼銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍏ュ彛浣嶄簬鎺у埗娴佹牴閮紝璋冪敤椤哄簭鍐冲畾鍚庣画璧勬簮鐢熷懡鍛ㄦ湡鍜屾暟鎹緷璧栥€?int main(int argc, char** argv)
int main(int argc, char** argv)
{
  nvapp::ApplicationCreateInfo appInfo;//搴旂敤绋嬪簭鍒涘缓淇℃伅缁撴瀯浣擄紝鍖呭惈 Vulkan 涓婁笅鏂囬厤缃€佺獥鍙ｈ缃€乁I 閫夐」鍜屽叾浠栧叏灞€鍙傛暟銆?  appInfo.name    = TARGET_NAME;//璁剧疆搴旂敤绋嬪簭鍚嶇О
  appInfo.name    = TARGET_NAME;
  appInfo.useMenu = true;//鍚敤鑿滃崟
  appInfo.vSync = false;//绂佺敤鍨傜洿鍚屾浠ヨ幏寰楁洿楂樼殑甯х巼锛岄€傚悎鎬ц兘娴嬭瘯鍜屽熀鍑嗚瘎娴?  VkPhysicalDeviceShaderSMBuiltinsFeaturesNV smNV = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_FEATURES_NV};//澹版槑涓€涓粨鏋勪綋瀹炰緥锛岀敤浜庢煡璇㈠拰鍚敤 NVIDIA Shader SM Builtins 鎵╁睍鐨勫姛鑳斤紝鍒濆鍖?sType 瀛楁浠ヤ究鍚庣画閾惧紡缁撴瀯浣跨敤
  VkPhysicalDeviceShaderSMBuiltinsFeaturesNV smNV = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SM_BUILTINS_FEATURES_NV};
  VkPhysicalDeviceMeshShaderFeaturesNV       meshNV  = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV};//澹版槑涓€涓粨鏋勪綋瀹炰緥锛岀敤浜庢煡璇㈠拰鍚敤 NVIDIA Mesh Shader 鎵╁睍鐨勫姛鑳斤紝鍒濆鍖?sType 瀛楁浠ヤ究鍚庣画閾惧紡缁撴瀯浣跨敤
  VkPhysicalDeviceMeshShaderFeaturesEXT      meshEXT = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT};//澹版槑涓€涓粨鏋勪綋瀹炰緥锛岀敤浜庢煡璇㈠拰鍚敤閫氱敤 Mesh Shader EXT 鎵╁睍鐨勫姛鑳斤紝鍒濆鍖?sType 瀛楁浠ヤ究鍚庣画閾惧紡缁撴瀯浣跨敤
  VkPhysicalDeviceShaderClockFeaturesKHR clockKHR = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR};//澹版槑涓€涓粨鏋勪綋瀹炰緥锛岀敤浜庢煡璇㈠拰鍚敤鐫€鑹插櫒鏃堕挓 KHR 鎵╁睍鐨勫姛鑳斤紝鍒濆鍖?sType 瀛楁浠ヤ究鍚庣画閾惧紡缁撴瀯浣跨敤
  VkPhysicalDeviceShaderAtomicFloatFeaturesEXT atomicFloatFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT};
  VkPhysicalDeviceFragmentShadingRateFeaturesKHR shadingRateFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR};
  VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR barycentricFeatures{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR};
  VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT shaderImageAtomic64Features{
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_ATOMIC_INT64_FEATURES_EXT};

  nvvk::ContextInitInfo vkSetup{
      .instanceExtensions = {VK_EXT_DEBUG_UTILS_EXTENSION_NAME},//瀹炰緥灞傞潰鍚敤璋冭瘯宸ュ叿鎵╁睍
      .deviceExtensions   = {{VK_KHR_SWAPCHAIN_EXTENSION_NAME}},//鏄惧崱鏄惁鏀寔 Swapchain 鎵╁睍
      .queues             = {VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_TRANSFER_BIT},//璇锋眰鍥惧舰鍜屼紶杈撻槦鍒?  };
  };

  vkSetup.deviceExtensions.push_back({VK_EXT_MESH_SHADER_EXTENSION_NAME, &meshEXT});//璇锋眰 Mesh Shader EXT 鎵╁睍浠ユ敮鎸佹洿骞挎硾鐨勮澶囷紝NV Mesh Shader 鏄叾瀛愰泦
  vkSetup.deviceExtensions.push_back({VK_KHR_SHADER_CLOCK_EXTENSION_NAME, &clockKHR});//璇锋眰 Shader Clock 鎵╁睍浠ユ敮鎸佺潃鑹插櫒鏃堕挓鍔熻兘
  vkSetup.deviceExtensions.push_back({VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME, &atomicFloatFeatures});//璇锋眰 Shader Atomic Float 鎵╁睍浠ユ敮鎸佺潃鑹插櫒鍘熷瓙娴偣鎿嶄綔


  vkSetup.deviceExtensions.push_back({VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME, &shadingRateFeatures});//璇锋眰 Fragment Shading Rate 鎵╁睍浠ユ敮鎸佸彲鍙橀€熺巼鐫€鑹插櫒鍔熻兘

  vkSetup.deviceExtensions.push_back({VK_EXT_SHADER_IMAGE_ATOMIC_INT64_EXTENSION_NAME, &shaderImageAtomic64Features});//璇锋眰 Shader Image Atomic Int64 鎵╁睍浠ユ敮鎸佺潃鑹插櫒鍥惧儚鍘熷瓙64浣嶆搷浣?
#if 1



  vkSetup.deviceExtensions.push_back({VK_NV_SHADER_SM_BUILTINS_EXTENSION_NAME, &smNV, false});
  vkSetup.deviceExtensions.push_back({VK_NV_MESH_SHADER_EXTENSION_NAME, &meshNV, false});

  vkSetup.deviceExtensions.push_back({VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME, &barycentricFeatures, false});
#endif


  nvutils::ProfilerManager                    profilerManager;
  std::shared_ptr<nvutils::CameraManipulator> cameraManipulator = std::make_shared<nvutils::CameraManipulator>();

  nvutils::ParameterRegistry            parameterRegistry;
  nvutils::ParameterParser              parameterParser;
  nvutils::ParameterSequencer::InitInfo sequencerInfo{
                                                      .parameterParser   = &parameterParser,
                                                      .parameterRegistry = &parameterRegistry,

                                                      .profilerManager = &profilerManager};

  nvvk::ValidationSettings::LayerPresets validationPreset = nvvk::ValidationSettings::LayerPresets::eStandard;

  parameterRegistry.add({"validation"}, &vkSetup.enableValidationLayers);
  parameterRegistry.add({"validationpreset"}, (int*)&validationPreset);
  parameterRegistry.add({"vsync"}, &appInfo.vSync);
  parameterRegistry.add({"device", "force a vulkan device via index into the device list"}, &vkSetup.forceGPU);
  parameterRegistry.add({"headless"}, &appInfo.headless, true);
  parameterRegistry.add({"headlessframes"}, &appInfo.headlessFrameCount);

  LodClusters::Info sampleInfo;
  sampleInfo.cameraManipulator               = cameraManipulator;
  sampleInfo.profilerManager                 = &profilerManager;
  sampleInfo.parameterRegistry               = &parameterRegistry;
  sampleInfo.parameterParser                 = &parameterParser;
  std::shared_ptr<LodClusters> sampleElement = std::make_shared<LodClusters>(sampleInfo);


  sequencerInfo.registerScriptParameters(parameterRegistry, parameterParser);


  sequencerInfo.postCallbacks.emplace_back(
      [&](const nvutils::ParameterSequencer::State& state) { sampleElement->parameterSequenceCallback(state); });


  parameterParser.add(parameterRegistry);

  parameterParser.setVerbose(true);

  parameterParser.parse(argc, argv);


  auto elemSequencer = std::make_shared<nvapp::ElementSequencer>(sequencerInfo);

  if(sampleElement->isCacheLoadOnly())
  {
    sampleElement->doCacheLoadOnly();
    return 0;
  }


  if(sampleElement->isProcessingOnly())
  {

    sampleElement->doProcessingOnly();
    return 0;
  }

  nvvk::ValidationSettings validationSettings;
  if(vkSetup.enableValidationLayers)
  {

    validationSettings.setPreset(validationPreset);
    validationSettings.duplicate_message_limit = 3;
    validationSettings.message_id_filter = {"VUID-RuntimeSpirv-storageInputOutput16-06334", "VUID-VkShaderModuleCreateInfo-pCode-08740"};


    vkSetup.instanceCreateInfoExt = validationSettings.buildPNextChain();
  }


  nvvk::addSurfaceExtensions(vkSetup.instanceExtensions);
  nvvk::Context vkContext;


  NVVK_CHECK(volkInitialize());

  {


    // 鍑芥暟锛歴t銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?    // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?    // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?    nvutils::ScopedTimer st("Creating Vulkan Context");
    nvutils::ScopedTimer st("Creating Vulkan Context");
    VkResult result{};
    vkContext.contextInfo = vkSetup;

    result = vkContext.createInstance();

    result = vkContext.selectPhysicalDevice();

    result = vkContext.createDevice();

    NVVK_CHECK(result);
    nvvk::DebugUtil::getInstance().init(vkContext.getDevice());
    if(vkContext.contextInfo.verbose)
    {
      NVVK_CHECK(nvvk::Context::printVulkanVersion());
      NVVK_CHECK(nvvk::Context::printInstanceLayers());
      NVVK_CHECK(nvvk::Context::printInstanceExtensions(vkContext.contextInfo.instanceExtensions));
      NVVK_CHECK(nvvk::Context::printDeviceExtensions(vkContext.getPhysicalDevice(), vkContext.contextInfo.deviceExtensions));
    }
    {
      NVVK_CHECK(nvvk::Context::printGpus(vkContext.getInstance(), vkContext.getPhysicalDevice()));

      LOGI("_________________________________________________\n");
    }
  }

  sampleElement->setSupportsBarycentrics(vkContext.hasExtensionEnabled(VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME));
  sampleElement->setSupportsMeshShaderNV(vkContext.hasExtensionEnabled(VK_NV_MESH_SHADER_EXTENSION_NAME));
  sampleElement->setSupportsSmBuiltinsNV(vkContext.hasExtensionEnabled(VK_NV_SHADER_SM_BUILTINS_EXTENSION_NAME));

  appInfo.instance       = vkContext.getInstance();

  appInfo.device         = vkContext.getDevice();

  appInfo.physicalDevice = vkContext.getPhysicalDevice();

  appInfo.queues         = vkContext.getQueueInfos();


  bool hasDebugUI = sampleElement->getShowDebugUI();


  appInfo.dockSetup = [&hasDebugUI](ImGuiID viewportID) {
    if(hasDebugUI)
    {


      ImGuiID debugID = ImGui::DockBuilderSplitNode(viewportID, ImGuiDir_Left, 0.15F, nullptr, &viewportID);

      ImGui::DockBuilderDockWindow("Debug", debugID);
    }


    ImGuiID settingID = ImGui::DockBuilderSplitNode(viewportID, ImGuiDir_Right, 0.25F, nullptr, &viewportID);

    ImGui::DockBuilderDockWindow("Settings", settingID);

    ImGui::DockBuilderDockWindow("Misc Settings", settingID);


    ImGuiID loggerID = ImGui::DockBuilderSplitNode(viewportID, ImGuiDir_Down, 0.35F, nullptr, &viewportID);

    ImGui::DockBuilderDockWindow("Log", loggerID);

    ImGuiID profilerID = ImGui::DockBuilderSplitNode(loggerID, ImGuiDir_Right, 0.75F, nullptr, &loggerID);

    ImGui::DockBuilderDockWindow("Profiler", profilerID);

    ImGuiID streamingID = ImGui::DockBuilderSplitNode(profilerID, ImGuiDir_Right, 0.66F, nullptr, &profilerID);

    ImGui::DockBuilderDockWindow("Streaming memory", streamingID);

    ImGuiID statisticsID = ImGui::DockBuilderSplitNode(streamingID, ImGuiDir_Right, 0.5F, nullptr, &streamingID);

    ImGui::DockBuilderDockWindow("Statistics", statisticsID);
  };


  nvapp::Application app;

  app.init(appInfo);

  auto                  logger      = std::make_shared<nvapp::ElementLogger>();

  nvapp::ElementLogger* loggerDeref = logger.get();
  nvutils::Logger::getInstance().setLogCallback([&](nvutils::Logger::LogLevel logLevel, const std::string& text) {
    loggerDeref->addLog(logLevel, "%s", text.c_str());
  });

  auto profilerUiSettings          = std::make_shared<nvapp::ElementProfiler::ViewSettings>();
  profilerUiSettings->table.levels = 1u;

  app.addElement(elemSequencer);
  app.addElement(std::make_shared<nvapp::ElementDefaultWindowTitle>());

  app.addElement(sampleElement);

  app.addElement(logger);
  app.addElement(std::make_shared<nvapp::ElementCamera>(cameraManipulator));
  app.addElement(std::make_shared<nvapp::ElementProfiler>(&profilerManager, profilerUiSettings));

  app.run();

  nvutils::Logger::getInstance().setLogCallback(nullptr);


  app.deinit();

  vkContext.deinit();

  return 0;
}
