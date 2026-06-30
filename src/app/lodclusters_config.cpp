//==============================================================================
// 鏂囦欢锛歴rc/app/lodclusters_config.cpp
// 妯″潡瀹氫綅锛氭瀯閫犲嚱鏁板拰榛樿鍙傛暟娉ㄥ唽瀹炵幇锛屽畾涔夊簲鐢ㄥ彲浠庡懡浠よ銆侀厤缃枃浠跺拰 UI 璋冩暣鐨勪富瑕佸疄楠屽彉閲忋€?// 鏁版嵁娴侊細杈撳叆鏄?Info 涓殑鍙傛暟娉ㄥ唽鍣ㄣ€佽В鏋愬櫒鍜?鎬ц兘鍒嗘瀽鍣紱杈撳嚭鏄粯璁?FrameConfig銆丼ceneConfig銆丷endererConfig 涓?StreamingConfig銆?// 鏂规硶璇存槑锛氳鏂囦欢鎶婃覆鏌撳疄楠岀殑鎺у埗鍙橀噺鏄惧紡鍙傛暟鍖栵紝渚夸簬澶嶇幇瀹為獙銆佹壒澶勭悊搴忓垪鍜屾€ц兘瀵规瘮銆?// 姝ｇ‘鎬х害鏉燂細榛樿鍊煎繀椤讳笌 鐫€鑹插櫒 渚у父閲忓拰 UI 閫夐」淇濇寔涓€鑷达紱鏂板鍙傛暟闇€瑕佸悓姝ヨ€冭檻閰嶇疆鍔犺浇銆佸簭鍒楀寲鍜岀粺璁¤緭鍑恒€?// 娉ㄩ噴椋庢牸锛氫娇鐢ㄤ腑鏂囪В閲?CPU 渚ц涔夛紱淇濈暀蹇呰鐨?API銆佺被鍨嬪悕鍜屾暟瀛︾缉鍐欎互渚挎绱€?//==============================================================================
// 渚濊禆璇存槑锛氬紩鍏ユ湰缂栬瘧鍗曞厓闇€瑕佺殑澶栭儴搴撱€侀」鐩ā鍧楀拰鍏变韩鐫€鑹插櫒甯冨眬銆?// 渚濊禆椤哄簭閫氬父鍙嶆槧鎶借薄灞傛锛氬厛澶栭儴搴擄紝鍐嶉」鐩ā鍧楋紝鏈€鍚庝笌 GPU 鍏变韩鐨勬帴鍙ｅ畾涔夈€?#include "lodclusters.hpp"
#include "lodclusters.hpp"

bool g_verbose = false;


// 鍛藉悕绌洪棿璇存槑锛氶檺鍒剁鍙峰彲瑙佽寖鍥达紝骞惰〃鏄庤繖浜涚被鍨嬪拰鍑芥暟灞炰簬鍚屼竴鍔熻兘鍩熴€?// 璇ヨ竟鐣屾湁鍔╀簬鍖哄垎搴旂敤灞傘€佹覆鏌撳眰銆佸満鏅眰鍜岀畻娉曞眰鐨勮亴璐ｃ€?namespace lodclusters {
namespace lodclusters {


LodClusters::LodClusters(const Info& info)

    : m_info(info)
{
  nvutils::ProfilerTimeline::CreateInfo createInfo;
  createInfo.name = "graphics";

  m_profilerTimeline = m_info.profilerManager->createTimeline(createInfo);
  m_info.parameterRegistry->add({"scene"}, {".gltf", ".glb", ".cfg"}, &m_sceneFilePathDropNew);
  m_info.parameterRegistry->add({"renderer"}, (int*)&m_tweak.renderer);
  m_info.parameterRegistry->add({"verbose"}, &g_verbose, true);
  m_info.parameterRegistry->add({"resetstats"}, &m_tweak.autoResetTimers);
  m_info.parameterRegistry->add({"supersample"}, &m_tweak.supersample);
  m_info.parameterRegistry->add({"debugui"}, &m_showDebugUI);
  m_info.parameterRegistry->add({"sequencescreenshot", "save screenshot at end of each sequence. 0 disabled (default), 1 full window, 2 rendered viewport"},(int*)&m_sequenceScreenshotMode, true);
  m_info.parameterRegistry->add({"dumpspirv", "dumps compiled spirv into working directory"}, &m_resources.m_dumpSpirv);
  m_info.parameterRegistry->add({"camerastring"}, &m_cameraString);
  m_info.parameterRegistry->add({"cameraspeed"}, &m_cameraSpeed);
  m_info.parameterRegistry->addVector({"sundirection"}, &m_frameConfig.frameConstants.skyParams.sunDirection);
  m_info.parameterRegistry->addVector({"suncolor"}, &m_frameConfig.frameConstants.skyParams.sunColor);
  m_info.parameterRegistry->add({"streaming"}, &m_tweak.useStreaming);
  m_info.parameterRegistry->add({"gridcopies"}, &m_sceneGridConfig.numCopies);
  m_info.parameterRegistry->add({"modelarray"}, &m_sceneGridConfig.numCopies);
  m_info.parameterRegistry->add({"arraycopies"}, &m_sceneGridConfig.numCopies);
  m_info.parameterRegistry->add({"gridconfig"}, &m_sceneGridConfig.gridBits);
  m_info.parameterRegistry->add({"gridunique"}, &m_sceneGridConfig.uniqueGeometriesForCopies);
  m_info.parameterRegistry->add({"clusterconfig"}, (int*)&m_tweak.clusterConfig);
  m_info.parameterRegistry->add({"clustergroupsize"}, &m_sceneConfig.clusterGroupSize);
  m_info.parameterRegistry->add({"simplifyuvweight"}, &m_sceneConfig.simplifyTexCoordWeight);
  m_info.parameterRegistry->add({"simplifynormalweight"}, &m_sceneConfig.simplifyNormalWeight);
  m_info.parameterRegistry->add({"simplifytangentweight"}, &m_sceneConfig.simplifyTangentWeight);
  m_info.parameterRegistry->add({"simplifytangentsignweight"}, &m_sceneConfig.simplifyTangentSignWeight);
  m_info.parameterRegistry->add({"attributes"}, &m_sceneConfig.enabledAttributes);

  m_info.parameterRegistry->add({"loderrormergeprevious"}, &m_sceneConfig.lodErrorMergePrevious);
  m_info.parameterRegistry->add({"loderrormergeadditive"}, &m_sceneConfig.lodErrorMergeAdditive);
  m_info.parameterRegistry->add({"loderroredgelimit"}, &m_sceneConfig.lodErrorEdgeLimit);
  m_info.parameterRegistry->add({"lodnodewidth"}, &m_sceneConfig.preferredNodeWidth);
  m_info.parameterRegistry->add({"loddecimationfactor"}, &m_sceneConfig.lodLevelDecimationFactor);
  m_info.parameterRegistry->add({"assemblymininstances"}, &m_sceneConfig.assemblyCullingMinInstances);
  m_info.parameterRegistry->add({"assemblylodpixels"}, &m_sceneConfig.assemblyLodPixelThreshold);
  m_info.parameterRegistry->add({"meshoptfillweight"}, &m_sceneConfig.meshoptFillWeight);
  m_info.parameterRegistry->add({"featureconstraints"}, &m_sceneConfig.featureConstraints);
  m_info.parameterRegistry->add({"featureimportanceweight"}, &m_sceneConfig.featureImportanceWeight);
  m_info.parameterRegistry->add({"featureprotectthreshold"}, &m_sceneConfig.featureProtectThreshold);
  m_info.parameterRegistry->add({"featurecriticalthreshold"}, &m_sceneConfig.featureCriticalThreshold);

  m_info.parameterRegistry->add({"loderror"}, &m_frameConfig.lodPixelError);
  m_info.parameterRegistry->add({"shadowray"}, &m_frameConfig.frameConstants.doShadow);
  m_info.parameterRegistry->add({"maxtransfermegabytes"}, (uint32_t*)&m_streamingConfig.maxTransferMegaBytes);
  m_info.parameterRegistry->add({"maxgeomegabytes"}, (uint32_t*)&m_streamingConfig.maxGeometryMegaBytes);
  m_info.parameterRegistry->add({"maxresidentgroups"}, &m_streamingConfig.maxGroups);
  m_info.parameterRegistry->add({"maxframeloadrequests"}, &m_streamingConfig.maxPerFrameLoadRequests);
  m_info.parameterRegistry->add({"maxframeunloadrequests"}, &m_streamingConfig.maxPerFrameUnloadRequests);
  m_info.parameterRegistry->add({"cullederrorscale"}, &m_frameConfig.culledErrorScale);
  m_info.parameterRegistry->add({"culling"}, &m_rendererConfig.useCulling);

  m_info.parameterRegistry->add({"twopassculling"}, &m_rendererConfig.useTwoPassCulling);
  m_info.parameterRegistry->add({"adaptivetwopassculling"}, &m_rendererConfig.useAdaptiveTwoPassCulling);
  m_info.parameterRegistry->add({"twopassminclusters"}, &m_rendererConfig.twoPassMinClusters);
  m_info.parameterRegistry->add({"twopassmatrixdelta"}, &m_rendererConfig.twoPassMatrixDelta);
  m_info.parameterRegistry->add({"forcedinvisculling"}, &m_rendererConfig.useForcedInvisibleCulling);
  m_info.parameterRegistry->add({"separategroups"}, &m_rendererConfig.useSeparateGroups);
  m_info.parameterRegistry->add({"cachingenabledlevels"}, &m_frameConfig.cachingEnabledLevels);
  m_info.parameterRegistry->add({"instancesorting"}, &m_rendererConfig.useSorting);
  m_info.parameterRegistry->add({"renderclusterbits"}, &m_rendererConfig.numRenderClusterBits);
  m_info.parameterRegistry->add({"rendertraversalbits"}, &m_rendererConfig.numTraversalTaskBits);
  m_info.parameterRegistry->add({"visualize"}, &m_frameConfig.visualize);
  m_info.parameterRegistry->add({"renderstats"}, &m_rendererConfig.useRenderStats);
  m_info.parameterRegistry->add({"extmeshshader"}, &m_rendererConfig.useEXTmeshShader);
  m_info.parameterRegistry->add({"forcepreprocessmegabytes"}, (uint32_t*)&m_sceneLoaderConfig.forcePreprocessMiB);
  m_info.parameterRegistry->add({"facetshading"}, &m_tweak.facetShading);
  m_info.parameterRegistry->add({"flipwinding"}, &m_rendererConfig.flipWinding);
  m_info.parameterRegistry->add({"forcetwosided"}, &m_rendererConfig.forceTwoSided);
  m_info.parameterRegistry->add({"autosavecache", "automatically store cache file for loaded scene. default true"},&m_sceneLoaderConfig.autoSaveCache);
  m_info.parameterRegistry->add({"autoloadcache", "automatically load cache file if found. default true"},&m_sceneLoaderConfig.autoLoadCache);
  m_info.parameterRegistry->add({"mappedcache", "work from memory mapped cache file, otherwise load to sysmem. default false"},&m_sceneLoaderConfig.memoryMappedCache);
  m_info.parameterRegistry->add({"processingonly", "directly terminate app once cache file was saved. default false"},&m_sceneLoaderConfig.processingOnly);
  m_info.parameterRegistry->add({"cacheloadonly", "load an existing scene cache and terminate before creating a Vulkan context. default false"},&m_cacheLoadOnly);
  m_info.parameterRegistry->add({"processingpartial", "in processingonly mode also allow partial/resuming processing. default false"},&m_sceneLoaderConfig.processingAllowPartial);
  m_info.parameterRegistry->add({"processingmode", "0 auto, -1 inner (within geometry), +1 outer (over geometries) parallelism. default 0"},&m_sceneLoaderConfig.processingMode);
  m_info.parameterRegistry->add({"processingthreadpct", "float percentage of threads during initial file load and processing into lod clusters, default 0.5 == 50 %"},&m_sceneLoaderConfig.processingThreadsPct);
  m_info.parameterRegistry->add({"compressed"}, &m_sceneConfig.useCompressedData);
  m_info.parameterRegistry->add({"compressedpositionbits"}, &m_sceneConfig.compressionPosDropBits);
  m_info.parameterRegistry->add({"compressedtexcoordbits"}, &m_sceneConfig.compressionTexDropBits);
  m_info.parameterRegistry->add({"cachesuffix", "default is .zippp"}, &m_sceneCacheSuffix);
  {

    static bool dummy;
    m_info.parameterRegistry->add({"twosided", "deprecated - now detecting doubleSided materials - there is a new forcetwosided"},
                                  &dummy);
  }

  m_frameConfig.frameConstants                         = {};
  m_frameConfig.frameConstants.wireThickness           = 2.f;
  m_frameConfig.frameConstants.wireSmoothing           = 1.f;
  m_frameConfig.frameConstants.wireColor               = {118.f / 255.f, 185.f / 255.f, 0.f};
  m_frameConfig.frameConstants.wireStipple             = 0;
  m_frameConfig.frameConstants.wireBackfaceColor       = {0.5f, 0.5f, 0.5f};
  m_frameConfig.frameConstants.wireStippleRepeats      = 5;
  m_frameConfig.frameConstants.wireStippleLength       = 0.5f;
  m_frameConfig.frameConstants.doShadow                = 1;
  m_frameConfig.frameConstants.doWireframe             = 0;
  m_frameConfig.frameConstants.ambientOcclusionRadius  = 0.1f;
  m_frameConfig.frameConstants.ambientOcclusionSamples = 2;
  m_frameConfig.frameConstants.visualize               = VISUALIZE_LOD;
  m_frameConfig.frameConstants.facetShading            = 1;
  m_frameConfig.frameConstants.lightMixer = 0.5f;
  m_frameConfig.frameConstants.skyParams  = {};
  m_frameConfig.frameConstants.time = 0.0f;
  m_frameConfig.frameConstants.deltaTime = 0.0f;
  m_frameConfig.frameConstants.lodTransitionSpeed = 1.0f;
  m_lastAmbientOcclusionSamples = m_frameConfig.frameConstants.ambientOcclusionSamples;
  m_sceneLoaderConfig.progressPct = &m_sceneProgress;
}

}
