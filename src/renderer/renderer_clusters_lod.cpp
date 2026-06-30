//==============================================================================
// 鏂囦欢锛歴rc/renderer/renderer_clusters_lod.cpp
// 妯″潡瀹氫綅锛氱皣 LOD 涓绘覆鏌撳櫒瀹炵幇锛岀粍缁囬亶鍘嗐€佹瀯寤洪棿鎺ュ懡浠ゃ€佺‖浠剁綉鏍肩潃鑹插櫒銆佽蒋浠跺厜鏍呫€丠i-Z 鍜?娴佸紡鍔犺浇銆?// 鏁版嵁娴侊細杈撳叆鏄?FrameConfig銆丷enderScene 鍜屼笂涓€甯ч伄鎸′俊鎭紱杈撳嚭鏄湰甯у彲瑙?绨?鍒楄〃銆佺粯鍒跺懡浠ゃ€佸洖璇绘暟鎹?鍜?Hi-Z銆?// 鏂规硶璇存槑锛氳鏂囦欢瀹炵幇 GPU 椹卞姩鐨勫眰娆?LOD 娓叉煋锛氳绠楅樁娈甸€夋嫨鍙绨囷紝鍥惧舰/璁＄畻鍏夋爡闃舵骞惰杈撳嚭锛孒i-Z 鍦ㄥ抚闂村舰鎴愰伄鎸″弽棣堛€?// 姝ｇ‘鎬х害鏉燂細闃舵 椤哄簭蹇呴』婊¤冻鏁版嵁渚濊禆锛涢棿鎺ヨ鏁板繀椤诲湪 璋冨害/缁樺埗 鍓嶅啓瀹岋紱two-闃舵 culling 瑕佹纭淮鎶や笂涓€閬嶅彲瑙佹€с€?// 娉ㄩ噴椋庢牸锛氫娇鐢ㄤ腑鏂囪В閲?CPU 渚ц涔夛紱淇濈暀蹇呰鐨?API銆佺被鍨嬪悕鍜屾暟瀛︾缉鍐欎互渚挎绱€?//==============================================================================
// 渚濊禆璇存槑锛氬紩鍏ユ湰缂栬瘧鍗曞厓闇€瑕佺殑澶栭儴搴撱€侀」鐩ā鍧楀拰鍏变韩鐫€鑹插櫒甯冨眬銆?// 渚濊禆椤哄簭閫氬父鍙嶆槧鎶借薄灞傛锛氬厛澶栭儴搴擄紝鍐嶉」鐩ā鍧楋紝鏈€鍚庝笌 GPU 鍏变韩鐨勬帴鍙ｅ畾涔夈€?#include <volk.h>
#include <volk.h>
#include <nvutils/alignment.hpp>
#include <fmt/format.h>
#include <algorithm>
#include <cmath>
#include "renderer.hpp"
#include "shaderio.h"

namespace {
float matrixMaxAbsDelta(const glm::mat4& a, const glm::mat4& b)
{
  float maxDelta = 0.0f;
  for(int col = 0; col < 4; col++)
  {
    for(int row = 0; row < 4; row++)
    {
      maxDelta = std::max(maxDelta, std::abs(a[col][row] - b[col][row]));
    }
  }
  return maxDelta;
}
}  // namespace


// 鍛藉悕绌洪棿璇存槑锛氶檺鍒剁鍙峰彲瑙佽寖鍥达紝骞惰〃鏄庤繖浜涚被鍨嬪拰鍑芥暟灞炰簬鍚屼竴鍔熻兘鍩熴€?// 璇ヨ竟鐣屾湁鍔╀簬鍖哄垎搴旂敤灞傘€佹覆鏌撳眰銆佸満鏅眰鍜岀畻娉曞眰鐨勮亴璐ｃ€?namespace lodclusters {
namespace lodclusters {


// 绫诲瀷锛歊endererRasterClustersLod銆傚皝瑁呮湰妯″潡鐨勯暱鏈熺姸鎬併€佽祫婧愭墍鏈夋潈鍜屽澶栨搷浣滄帴鍙ｃ€?// 璁捐鎰忓浘锛氶€氳繃鎴愬憳鍑芥暟闆嗕腑缁存姢鐘舵€佽浆绉伙紝閬垮厤璋冪敤鏂圭洿鎺ユ嫾鎺ュ簳灞傝祫婧愮敓鍛藉懆鏈熴€?// 浣跨敤绾︽潫锛氬疄渚嬪垵濮嬪寲銆佹瘡甯т娇鐢ㄥ拰閲婃斁搴旈伒瀹堝０鏄庨『搴忓搴旂殑渚濊禆鍏崇郴銆?class RendererRasterClustersLod : public Renderer
class RendererRasterClustersLod : public Renderer
{
public:

  virtual bool init(Resources& res, RenderScene& rscene, const RendererConfig& config) override;
  virtual void render(VkCommandBuffer primary, Resources& res, RenderScene& rscene, const FrameConfig& frame, nvvk::ProfilerGpuTimer& profiler) override;
  virtual void updatedFrameBuffer(Resources& res, RenderScene& rscene) override;
  virtual void deinit(Resources& res) override;

  // 鍑芥暟锛歩nit銆傚垵濮嬪寲鏈ā鍧楁墍闇€鐘舵€併€佽祫婧愭垨 GPU 渚х粦瀹氥€?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氬垵濮嬪寲杩囩▼寤虹珛鍚庣画闃舵鍋囧畾瀛樺湪鐨勪笉鍙橀噺锛屼緥濡傚彞鏌勬湁鏁堛€佺紦鍐插ぇ灏忚冻澶熴€佹弿杩扮宸茬粦瀹氥€?  virtual bool init(Resources& res, RenderScene& rscene, const RendererConfig& config) override;


  // 鍑芥暟锛歳ender銆傚綍鍒舵垨鎵ц娓叉煋鐩稿叧宸ヤ綔锛屾妸鍑嗗濂界殑鏁版嵁鎻愪氦鍒板綋鍓嶆覆鏌撻樁娈点€?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氭覆鏌撳嚱鏁伴€氬父澶勪簬甯х骇鍏抽敭璺緞锛屽繀椤诲皧閲嶅墠搴忚绠楅樁娈靛啓鍑虹殑璁℃暟銆佸湴鍧€鍜屽悓姝ュ睆闅溿€?  virtual void render(VkCommandBuffer primary, Resources& res, RenderScene& rscene, const FrameConfig& frame, nvvk::ProfilerGpuTimer& profiler) override;


  // 鍑芥暟锛歶pdatedFrameBuffer銆傛牴鎹渶鏂扮姸鎬佸埛鏂扮紦瀛樻暟鎹€丟PU 鍦板潃銆佹弿杩扮鎴栫粺璁′俊鎭€?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氭洿鏂板嚱鏁拌礋璐ｆ妸鈥滄棫鐘舵€佲€濇帹杩涘埌鈥滃綋鍓嶇姸鎬佲€濓紝鍥犳瑕侀伩鍏嶉儴鍒嗘洿鏂伴€犳垚 CPU/GPU 瑙嗗浘涓嶄竴鑷淬€?  virtual void updatedFrameBuffer(Resources& res, RenderScene& rscene) override;


  // 鍑芥暟锛歞einit銆傞噴鏀炬垨鍥炴敹鍓嶉潰鍒濆鍖栫殑璧勬簮锛屼繚鎸佺敓鍛藉懆鏈熸垚瀵圭鐞嗐€?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氶噴鏀鹃『搴忚閬靛畧璧勬簮渚濊禆鍏崇郴锛岄伩鍏?GPU 浠嶅彲鑳借闂殑瀵硅薄琚彁鍓嶉攢姣併€?  virtual void deinit(Resources& res) override;
private:


  // 鍑芥暟锛歩nitShaders銆傚垵濮嬪寲鏈ā鍧楁墍闇€鐘舵€併€佽祫婧愭垨 GPU 渚х粦瀹氥€?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氬垵濮嬪寲杩囩▼寤虹珛鍚庣画闃舵鍋囧畾瀛樺湪鐨勪笉鍙橀噺锛屼緥濡傚彞鏌勬湁鏁堛€佺紦鍐插ぇ灏忚冻澶熴€佹弿杩扮宸茬粦瀹氥€?  bool initShaders(Resources& res, RenderScene& scene, const RendererConfig& config);
  bool initShaders(Resources& res, RenderScene& scene, const RendererConfig& config);


  // 缁撴瀯锛歋haders銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?  // 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?  // 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?  struct Shaders
  struct Shaders
  {
    shaderc::SpvCompilationResult graphicsMesh;
    shaderc::SpvCompilationResult graphicsFragment;
    shaderc::SpvCompilationResult computeTraversalPresort;
    shaderc::SpvCompilationResult computeAssemblyVisibility;
    shaderc::SpvCompilationResult computeTraversalInit;
    shaderc::SpvCompilationResult computeTraversalRun;
    shaderc::SpvCompilationResult computeTraversalGroups;
    shaderc::SpvCompilationResult computeBuildSetup;
  };


  // 缁撴瀯锛歅ipelines銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?  // 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?  // 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?  struct Pipelines
  struct Pipelines
  {
    VkPipeline graphicsMesh            = nullptr;
    VkPipeline graphicsBboxes          = nullptr;
    VkPipeline computeTraversalPresort = nullptr;
    VkPipeline computeAssemblyVisibility = nullptr;
    VkPipeline computeTraversalInit    = nullptr;
    VkPipeline computeTraversalRun     = nullptr;
    VkPipeline computeTraversalGroups  = nullptr;
    VkPipeline computeBuildSetup       = nullptr;
  };
  Shaders            m_shaders;
  Pipelines          m_pipelines;
  VkShaderStageFlags m_stageFlags{};
  VkPipelineLayout   m_pipelineLayout{};
  nvvk::DescriptorPack m_dsetPack;
  nvvk::Buffer m_sceneBuildBuffer;
  nvvk::Buffer m_sceneTraversalBuffer;
  nvvk::Buffer m_sceneDataBuffer;
  nvvk::Buffer m_assemblyNodeBuffer;
  nvvk::Buffer m_assemblyStateBuffer;
  shaderio::SceneBuilding m_sceneBuildShaderio;
};


// 鍑芥暟锛歊endererRasterClustersLod::initShaders銆傚垵濮嬪寲鏈ā鍧楁墍闇€鐘舵€併€佽祫婧愭垨 GPU 渚х粦瀹氥€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氬垵濮嬪寲杩囩▼寤虹珛鍚庣画闃舵鍋囧畾瀛樺湪鐨勪笉鍙橀噺锛屼緥濡傚彞鏌勬湁鏁堛€佺紦鍐插ぇ灏忚冻澶熴€佹弿杩扮宸茬粦瀹氥€?bool RendererRasterClustersLod::initShaders(Resources& res, RenderScene& rscene, const RendererConfig& config)
bool RendererRasterClustersLod::initShaders(Resources& res, RenderScene& rscene, const RendererConfig& config)
{

  if(!initBasicShaders(res, rscene, config))
  {
    return false;
  }


  shaderc::CompileOptions options = res.makeCompilerOptions();


  uint32_t meshletTriangles = shaderio::adjustClusterProperty(rscene.scene->m_maxClusterTriangles);

  uint32_t meshletVertices  = shaderio::adjustClusterProperty(rscene.scene->m_maxClusterVertices);

  LOGI("mesh shader config: %d triangles %d vertices\n", meshletTriangles, meshletVertices);


  options.SetOptimizationLevel(shaderc_optimization_level_performance);


  options.AddMacroDefinition("SUBGROUP_SIZE", fmt::format("{}", res.m_physicalDeviceInfo.properties11.subgroupSize));
  options.AddMacroDefinition("USE_16BIT_DISPATCH", fmt::format("{}", res.m_use16bitDispatch ? 1 : 0));
  options.AddMacroDefinition("CLUSTER_VERTEX_COUNT", fmt::format("{}", meshletVertices));
  options.AddMacroDefinition("CLUSTER_TRIANGLE_COUNT", fmt::format("{}", meshletTriangles));

  options.AddMacroDefinition("TARGETS_RASTERIZATION", "1");

  options.AddMacroDefinition("USE_STREAMING", rscene.useStreaming ? "1" : "0");

  options.AddMacroDefinition("USE_SORTING", config.useSorting ? "1" : "0");

  options.AddMacroDefinition("USE_CULLING", config.useCulling ? "1" : "0");


  options.AddMacroDefinition("USE_TWO_PASS_CULLING", config.useCulling && config.useTwoPassCulling ? "1" : "0");

  options.AddMacroDefinition("USE_RENDER_STATS", config.useRenderStats ? "1" : "0");

  options.AddMacroDefinition("USE_SEPARATE_GROUPS", config.useSeparateGroups ? "1" : "0");
  options.AddMacroDefinition("USE_EXT_MESH_SHADER", fmt::format("{}", config.useEXTmeshShader ? 1 : 0));
  options.AddMacroDefinition("MESHSHADER_WORKGROUP_SIZE", fmt::format("{}", m_meshShaderWorkgroupSize));
  options.AddMacroDefinition("MESHSHADER_BBOX_COUNT", fmt::format("{}", m_meshShaderBoxes));


  options.AddMacroDefinition("ALLOW_VERTEX_NORMALS", rscene.scene->m_hasVertexNormals && res.m_supportsBarycentrics ? "1" : "0");

  options.AddMacroDefinition("ALLOW_VERTEX_TANGENTS", rscene.scene->m_hasVertexTangents && res.m_supportsBarycentrics ? "1" : "0");

  options.AddMacroDefinition("ALLOW_VERTEX_TEXCOORDS", rscene.scene->m_hasVertexTexCoord0 ? "1" : "0");


  options.AddMacroDefinition("ALLOW_SHADING", config.useShading ? "1" : "0");


  options.AddMacroDefinition("USE_DEPTH_ONLY", !config.useShading && config.useDepthOnly ? "1" : "0");

  options.AddMacroDefinition("DEBUG_VISUALIZATION", config.useDebugVisualization && res.m_supportsBarycentrics ? "1" : "0");

  options.AddMacroDefinition("USE_TWO_SIDED", rscene.scene->m_hasTwoSided && !config.forceTwoSided ? "1" : "0");

  options.AddMacroDefinition("USE_FORCED_TWO_SIDED", config.forceTwoSided ? "1" : "0");

  options.AddMacroDefinition("USE_FORCED_INVISIBLE_CULLING", "0");


    res.compileShader(m_shaders.graphicsMesh, VK_SHADER_STAGE_MESH_BIT_NV, "render/clusters.mesh.glsl", &options);

    res.compileShader(m_shaders.graphicsFragment, VK_SHADER_STAGE_FRAGMENT_BIT, "render/frag.glsl", &options);

    res.compileShader(m_shaders.computeTraversalPresort, VK_SHADER_STAGE_COMPUTE_BIT, "traversal/traversal_presort.comp.glsl", &options);

    res.compileShader(m_shaders.computeAssemblyVisibility, VK_SHADER_STAGE_COMPUTE_BIT, "traversal/assembly_visibility.comp.glsl", &options);

    res.compileShader(m_shaders.computeTraversalInit, VK_SHADER_STAGE_COMPUTE_BIT, "traversal/traversal_init.comp.glsl", &options);

    res.compileShader(m_shaders.computeTraversalRun, VK_SHADER_STAGE_COMPUTE_BIT, "traversal/traversal_run.comp.glsl", &options);

    res.compileShader(m_shaders.computeBuildSetup, VK_SHADER_STAGE_COMPUTE_BIT, "build/build_setup.comp.glsl", &options);

  if(config.useSeparateGroups)
  {

    res.compileShader(m_shaders.computeTraversalGroups, VK_SHADER_STAGE_COMPUTE_BIT,"traversal/traversal_run_separate_groups.comp.glsl", &options);
  }

  return res.verifyShaders(m_shaders);
}


// 鍑芥暟锛歊endererRasterClustersLod::init銆傚垵濮嬪寲鏈ā鍧楁墍闇€鐘舵€併€佽祫婧愭垨 GPU 渚х粦瀹氥€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氬垵濮嬪寲杩囩▼寤虹珛鍚庣画闃舵鍋囧畾瀛樺湪鐨勪笉鍙橀噺锛屼緥濡傚彞鏌勬湁鏁堛€佺紦鍐插ぇ灏忚冻澶熴€佹弿杩扮宸茬粦瀹氥€?bool RendererRasterClustersLod::init(Resources& res, RenderScene& rscene, const RendererConfig& config)
bool RendererRasterClustersLod::init(Resources& res, RenderScene& rscene, const RendererConfig& config)
{
  m_resourceReservedUsage = {};
  m_config                = config;
  m_maxRenderClusters     = 1u << config.numRenderClusterBits;
  m_maxTraversalTasks     = 1u << config.numTraversalTaskBits;

  if(!initShaders(res, rscene, config))
  {
    return false;
  }


  m_pipelines = {};

  m_dsetPack.deinit();


  initBasics(res, rscene, config);


  m_resourceReservedUsage.geometryMemBytes   = rscene.getGeometrySize(true);
  m_resourceReservedUsage.operationsMemBytes = logMemoryUsage(rscene.getOperationsSize(), "operations", "rscene total");
  {

    res.createBuffer(m_sceneBuildBuffer, sizeof(shaderio::SceneBuilding), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);

    NVVK_DBG_NAME(m_sceneBuildBuffer.buffer);

    m_resourceReservedUsage.operationsMemBytes += logMemoryUsage(m_sceneBuildBuffer.bufferSize, "operations", "build shaderio");

    memset(&m_sceneBuildShaderio, 0, sizeof(m_sceneBuildShaderio));
    m_sceneBuildShaderio.numRenderInstances = uint32_t(m_renderInstances.size());
    m_sceneBuildShaderio.numAssemblyNodes   = uint32_t(rscene.scene->m_assemblyNodes.size());
    m_sceneBuildShaderio.assemblyCullingMinInstances = rscene.scene->m_config.assemblyCullingMinInstances;
    m_sceneBuildShaderio.assemblyLodPixelThreshold = rscene.scene->m_config.assemblyLodPixelThreshold;

    m_sceneBuildShaderio.maxRenderClusters = uint32_t(1u << config.numRenderClusterBits);

    m_sceneBuildShaderio.maxTraversalInfos = uint32_t(1u << config.numTraversalTaskBits);

    m_sceneBuildShaderio.indirectDispatchGroups.gridY  = 1;
    m_sceneBuildShaderio.indirectDispatchGroups.gridZ  = 1;
    m_sceneBuildShaderio.indirectDrawClustersEXT.gridZ = 1;
    m_sceneBuildShaderio.indirectDrawClustersNV.first  = 0;

    if(m_sceneBuildShaderio.numAssemblyNodes)
    {
      const size_t assemblyBytes = sizeof(shaderio::AssemblyNode) * rscene.scene->m_assemblyNodes.size();
      res.createBuffer(m_assemblyNodeBuffer, assemblyBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
      NVVK_DBG_NAME(m_assemblyNodeBuffer.buffer);
      res.simpleUploadBuffer(m_assemblyNodeBuffer, const_cast<shaderio::AssemblyNode*>(rscene.scene->m_assemblyNodes.data()));
      m_sceneBuildShaderio.assemblyNodes = m_assemblyNodeBuffer.address;
      m_resourceReservedUsage.operationsMemBytes += logMemoryUsage(m_assemblyNodeBuffer.bufferSize, "operations", "assembly nodes");

      const size_t assemblyStateBytes = sizeof(shaderio::AssemblyState) * rscene.scene->m_assemblyNodes.size();
      res.createBuffer(m_assemblyStateBuffer, assemblyStateBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
      NVVK_DBG_NAME(m_assemblyStateBuffer.buffer);
      m_sceneBuildShaderio.assemblyStates = m_assemblyStateBuffer.address;
      m_resourceReservedUsage.operationsMemBytes += logMemoryUsage(m_assemblyStateBuffer.bufferSize, "operations", "assembly states");
    }

    BufferRanges mem = {};

    m_sceneBuildShaderio.renderClusterInfos = mem.append(sizeof(shaderio::ClusterInfo) * m_sceneBuildShaderio.maxRenderClusters, 8);

    if(config.useSorting)
    {
      m_sceneBuildShaderio.instanceSortKeys   = mem.append(sizeof(uint32_t) * m_renderInstances.size(), 4);
      m_sceneBuildShaderio.instanceSortValues = mem.append(sizeof(uint32_t) * m_renderInstances.size(), 4);
    }

    if(config.useSeparateGroups)
    {
      m_sceneBuildShaderio.traversalGroupInfos = mem.append(sizeof(uint64_t) * m_sceneBuildShaderio.maxTraversalInfos, 8);
    }

    if(config.useTwoPassCulling && config.useCulling)
    {
      m_sceneBuildShaderio.instanceVisibility = mem.append(sizeof(uint8_t) * m_renderInstances.size(), 4);
    }

    res.createBuffer(m_sceneDataBuffer, mem.getSize(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    NVVK_DBG_NAME(m_sceneDataBuffer.buffer);

    m_resourceReservedUsage.operationsMemBytes += logMemoryUsage(m_sceneDataBuffer.bufferSize, "operations", "build data");

    m_sceneBuildShaderio.renderClusterInfos += m_sceneDataBuffer.address;
    m_sceneBuildShaderio.instanceSortKeys += m_sceneDataBuffer.address;
    m_sceneBuildShaderio.instanceSortValues += m_sceneDataBuffer.address;
    if(config.useSeparateGroups)
    {
      m_sceneBuildShaderio.traversalGroupInfos += m_sceneDataBuffer.address;
    }

    if(config.useTwoPassCulling && config.useCulling)
    {
      m_sceneBuildShaderio.instanceVisibility += m_sceneDataBuffer.address;
    }

    res.createBuffer(m_sceneTraversalBuffer, sizeof(uint64_t) * m_sceneBuildShaderio.maxTraversalInfos, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    NVVK_DBG_NAME(m_sceneTraversalBuffer.buffer);

    m_resourceReservedUsage.operationsMemBytes += logMemoryUsage(m_sceneTraversalBuffer.bufferSize, "operations", "build traversal");
    m_sceneBuildShaderio.traversalNodeInfos = m_sceneTraversalBuffer.address;
  }


  updateBasicDescriptors(res, rscene, &m_sceneBuildBuffer);

  if(rscene.useStreaming)
  {

    rscene.sceneStreaming.updateBindings(m_sceneBuildBuffer);
  }
  {

    m_stageFlags = VK_SHADER_STAGE_MESH_BIT_NV | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

    nvvk::DescriptorBindings bindings;

    bindings.addBinding(BINDINGS_FRAME_UBO, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, m_stageFlags);

    bindings.addBinding(BINDINGS_READBACK_SSBO, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, m_stageFlags);

    bindings.addBinding(BINDINGS_GEOMETRIES_SSBO, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, m_stageFlags);

    bindings.addBinding(BINDINGS_RENDERINSTANCES_SSBO, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, m_stageFlags);

    bindings.addBinding(BINDINGS_SCENEBUILDING_SSBO, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, m_stageFlags);

    bindings.addBinding(BINDINGS_SCENEBUILDING_UBO, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, m_stageFlags);


    bindings.addBinding(BINDINGS_HIZ_TEX, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, config.useCulling && config.useTwoPassCulling ? 2 : 1, m_stageFlags);

    if(rscene.useStreaming)
    {

      bindings.addBinding(BINDINGS_STREAMING_SSBO, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, m_stageFlags);

      bindings.addBinding(BINDINGS_STREAMING_UBO, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, m_stageFlags);
    }


    m_dsetPack.init(bindings, res.m_device);

    nvvk::createPipelineLayout(res.m_device, &m_pipelineLayout, {m_dsetPack.getLayout()}, {{m_stageFlags, 0, sizeof(uint32_t)}});

    nvvk::WriteSetContainer writeSets;
    writeSets.append(m_dsetPack.makeWrite(BINDINGS_FRAME_UBO), res.m_commonBuffers.frameConstants);
    writeSets.append(m_dsetPack.makeWrite(BINDINGS_READBACK_SSBO), &res.m_commonBuffers.readBack);
    writeSets.append(m_dsetPack.makeWrite(BINDINGS_GEOMETRIES_SSBO), rscene.getShaderGeometriesBuffer());
    writeSets.append(m_dsetPack.makeWrite(BINDINGS_RENDERINSTANCES_SSBO), m_renderInstanceBuffer);
    writeSets.append(m_dsetPack.makeWrite(BINDINGS_SCENEBUILDING_SSBO), m_sceneBuildBuffer);
    writeSets.append(m_dsetPack.makeWrite(BINDINGS_SCENEBUILDING_UBO), m_sceneBuildBuffer);


    writeSets.append(m_dsetPack.makeWrite(BINDINGS_HIZ_TEX, 0, 0), &res.m_hizUpdate[0].farImageInfo);
    if(config.useCulling && config.useTwoPassCulling)
    {
      writeSets.append(m_dsetPack.makeWrite(BINDINGS_HIZ_TEX, 0, 1), &res.m_hizUpdate[1].farImageInfo);
    }
    if(rscene.useStreaming)
    {
      writeSets.append(m_dsetPack.makeWrite(BINDINGS_STREAMING_SSBO), rscene.sceneStreaming.getShaderStreamingBuffer());
      writeSets.append(m_dsetPack.makeWrite(BINDINGS_STREAMING_UBO), rscene.sceneStreaming.getShaderStreamingBuffer());
    }

    vkUpdateDescriptorSets(res.m_device, uint32_t(writeSets.size()), writeSets.data(), 0, nullptr);
  }

  {
    nvvk::GraphicsPipelineCreator graphicsGen;
    nvvk::GraphicsPipelineState   state = res.m_basicGraphicsState;
    graphicsGen.pipelineInfo.layout                  = m_pipelineLayout;

    graphicsGen.renderingState.depthAttachmentFormat = res.m_frameBuffer.pipelineRenderingInfo.depthAttachmentFormat;
    graphicsGen.renderingState.stencilAttachmentFormat = res.m_frameBuffer.pipelineRenderingInfo.stencilAttachmentFormat;
    graphicsGen.colorFormats = {res.m_frameBuffer.colorFormat};
    state.rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    if (config.forceTwoSided)
    {
        state.rasterizationState.cullMode = VK_CULL_MODE_NONE;
    }
    graphicsGen.addShader(VK_SHADER_STAGE_MESH_BIT_NV, "main", nvvkglsl::GlslCompiler::getSpirvData(m_shaders.graphicsMesh));


    if(!m_config.useDepthOnly)
    {
      graphicsGen.addShader(VK_SHADER_STAGE_FRAGMENT_BIT, "main", nvvkglsl::GlslCompiler::getSpirvData(m_shaders.graphicsFragment));
    }


    graphicsGen.createGraphicsPipeline(res.m_device, nullptr, state, &m_pipelines.graphicsMesh);
  }

  {
    VkComputePipelineCreateInfo compInfo   = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    VkShaderModuleCreateInfo    shaderInfo = {};
    compInfo.stage                         = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    compInfo.stage.stage                   = VK_SHADER_STAGE_COMPUTE_BIT;
    compInfo.stage.pName                   = "main";
    compInfo.stage.pNext                   = &shaderInfo;
    compInfo.layout                        = m_pipelineLayout;

    if(config.useSorting)
    {

      shaderInfo = nvvkglsl::GlslCompiler::makeShaderModuleCreateInfo(m_shaders.computeTraversalPresort);

      vkCreateComputePipelines(res.m_device, nullptr, 1, &compInfo, nullptr, &m_pipelines.computeTraversalPresort);
    }


    shaderInfo = nvvkglsl::GlslCompiler::makeShaderModuleCreateInfo(m_shaders.computeAssemblyVisibility);

    vkCreateComputePipelines(res.m_device, nullptr, 1, &compInfo, nullptr, &m_pipelines.computeAssemblyVisibility);


    shaderInfo = nvvkglsl::GlslCompiler::makeShaderModuleCreateInfo(m_shaders.computeBuildSetup);

    vkCreateComputePipelines(res.m_device, nullptr, 1, &compInfo, nullptr, &m_pipelines.computeBuildSetup);


    shaderInfo = nvvkglsl::GlslCompiler::makeShaderModuleCreateInfo(m_shaders.computeTraversalInit);

    vkCreateComputePipelines(res.m_device, nullptr, 1, &compInfo, nullptr, &m_pipelines.computeTraversalInit);


    shaderInfo = nvvkglsl::GlslCompiler::makeShaderModuleCreateInfo(m_shaders.computeTraversalRun);

    vkCreateComputePipelines(res.m_device, nullptr, 1, &compInfo, nullptr, &m_pipelines.computeTraversalRun);

    if(config.useSeparateGroups)
    {

      shaderInfo = nvvkglsl::GlslCompiler::makeShaderModuleCreateInfo(m_shaders.computeTraversalGroups);

      vkCreateComputePipelines(res.m_device, nullptr, 1, &compInfo, nullptr, &m_pipelines.computeTraversalGroups);
    }
  }

  return true;
}


// 鍑芥暟锛歡etWorkGroupCount銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?static uint32_t getWorkGroupCount(uint32_t numThreads, uint32_t workGroupSize)
static uint32_t getWorkGroupCount(uint32_t numThreads, uint32_t workGroupSize)
{
  return (numThreads + workGroupSize - 1) / workGroupSize;
}


// 鍑芥暟锛歊endererRasterClustersLod::render銆傚綍鍒舵垨鎵ц娓叉煋鐩稿叧宸ヤ綔锛屾妸鍑嗗濂界殑鏁版嵁鎻愪氦鍒板綋鍓嶆覆鏌撻樁娈点€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氭覆鏌撳嚱鏁伴€氬父澶勪簬甯х骇鍏抽敭璺緞锛屽繀椤诲皧閲嶅墠搴忚绠楅樁娈靛啓鍑虹殑璁℃暟銆佸湴鍧€鍜屽悓姝ュ睆闅溿€?void RendererRasterClustersLod::render(VkCommandBuffer cmd, Resources& res, RenderScene& rscene, const FrameConfig& frame, nvvk::ProfilerGpuTimer& profiler)
void RendererRasterClustersLod::render(VkCommandBuffer cmd, Resources& res, RenderScene& rscene, const FrameConfig& frame, nvvk::ProfilerGpuTimer& profiler)
{
  VkMemoryBarrier memBarrier = { VK_STRUCTURE_TYPE_MEMORY_BARRIER };

  {


    glm::vec2 renderScale = res.getFramebufferWindow2RenderScale();

    float     pixelScale  = std::min(renderScale.x, renderScale.y);


    m_sceneBuildShaderio.errorOverDistanceThreshold = clusterLodErrorOverDistance(frame.lodPixelError * pixelScale, frame.frameConstants.fov, frame.frameConstants.viewportf.y);
  }

  m_sceneBuildShaderio.traversalViewMatrix    = frame.traversalViewMatrix;
  m_sceneBuildShaderio.cullViewProjMatrix     = frame.cullViewProjMatrix;
  m_sceneBuildShaderio.cullViewProjMatrixLast = frame.cullViewProjMatrixLast;
  m_sceneBuildShaderio.pass                    = 0;
  m_sceneBuildShaderio.frameIndex              = m_frameIndex;

  const bool twoPassRequested = m_config.useCulling && m_config.useTwoPassCulling;
  const bool sceneLargeEnough = !rscene.scene || rscene.scene->m_totalClustersCount >= m_config.twoPassMinClusters;
  const bool cameraMovedEnough =
      matrixMaxAbsDelta(frame.cullViewProjMatrix, frame.cullViewProjMatrixLast) >= m_config.twoPassMatrixDelta;
  const bool twoPassActive = twoPassRequested
                             && (!m_config.useAdaptiveTwoPassCulling
                                 || (!frame.freezeCulling && sceneLargeEnough && cameraMovedEnough));
  m_sceneBuildShaderio.twoPassCullingActive = twoPassActive ? 1u : 0u;

  vkCmdUpdateBuffer(cmd, res.m_commonBuffers.frameConstants.buffer, 0, sizeof(shaderio::FrameConstants), (const uint32_t*)&frame.frameConstants);
  vkCmdUpdateBuffer(cmd, m_sceneBuildBuffer.buffer, 0, sizeof(shaderio::SceneBuilding), (const uint32_t*)&m_sceneBuildShaderio);

  vkCmdFillBuffer(cmd, res.m_commonBuffers.readBack.buffer, 0, sizeof(shaderio::Readback), 0);


  vkCmdFillBuffer(cmd, m_sceneTraversalBuffer.buffer, 0, m_sceneTraversalBuffer.bufferSize, ~0);

  if(rscene.useStreaming)
  {
    SceneStreaming::FrameSettings settings;
    settings.ageThreshold = frame.streamingAgeThreshold;

    rscene.sceneStreaming.cmdBeginFrame(cmd, res.m_queueStates.primary, res.m_queueStates.transfer, settings, profiler);
  }


  memBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT;
  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       0, 1, &memBarrier, 0, nullptr, 0, nullptr);

  if(rscene.useStreaming)
  {

    rscene.sceneStreaming.cmdPreTraversal(cmd, profiler);


    memBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_DEPENDENCY_BY_REGION_BIT, 1, &memBarrier, 0, nullptr, 0, nullptr);
  }

  const uint32_t passCount = twoPassActive ? 2 : 1;
  const uint32_t lastPass  = passCount - 1;
  for(uint32_t pass = 0; pass < passCount; pass++)
  {
    {

      auto timerSection = profiler.cmdFrameSection(cmd, "Traversal Preparation");

      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1, m_dsetPack.getSetPtr(), 0, nullptr);

      if(pass == 0 && m_config.useSorting)
      {

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelines.computeTraversalPresort);
        res.cmdLinearDispatch(cmd, getWorkGroupCount(m_sceneBuildShaderio.numRenderInstances, TRAVERSAL_PRESORT_WORKGROUP));


        memBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_DEPENDENCY_BY_REGION_BIT, 1, &memBarrier, 0, nullptr, 0, nullptr);

        vrdxCmdSortKeyValue(cmd, res.m_vrdxSorter, m_sceneBuildShaderio.numRenderInstances, m_sceneDataBuffer.buffer,
                            m_sceneBuildShaderio.instanceSortKeys - m_sceneDataBuffer.address, m_sceneDataBuffer.buffer,
                            m_sceneBuildShaderio.instanceSortValues - m_sceneDataBuffer.address,
                            m_sortingAuxBuffer.buffer, 0, nullptr, 0);


        memBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_DEPENDENCY_BY_REGION_BIT, 1, &memBarrier, 0, nullptr, 0, nullptr);
      }

      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1, m_dsetPack.getSetPtr(), 0, nullptr);

      if(m_sceneBuildShaderio.numAssemblyNodes)
      {
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelines.computeAssemblyVisibility);
        res.cmdLinearDispatch(cmd, getWorkGroupCount(m_sceneBuildShaderio.numAssemblyNodes, ASSEMBLY_VISIBILITY_WORKGROUP));

        memBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             VK_DEPENDENCY_BY_REGION_BIT, 1, &memBarrier, 0, nullptr, 0, nullptr);
      }

      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelines.computeTraversalInit);
      res.cmdLinearDispatch(cmd, getWorkGroupCount(m_sceneBuildShaderio.numRenderInstances, TRAVERSAL_INIT_WORKGROUP));


      memBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
      memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_DEPENDENCY_BY_REGION_BIT, 1, &memBarrier, 0, nullptr, 0, nullptr);

      uint32_t buildSetupID = BUILD_SETUP_TRAVERSAL_RUN;

      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelines.computeBuildSetup);
      vkCmdPushConstants(cmd, m_pipelineLayout, m_stageFlags, 0, sizeof(uint32_t), &buildSetupID);

      vkCmdDispatch(cmd, 1, 1, 1);


      vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_DEPENDENCY_BY_REGION_BIT, 1, &memBarrier, 0, nullptr, 0, nullptr);
    }
    {

        auto timerSection = profiler.cmdFrameSection(cmd, "Traversal Run");


        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelines.computeTraversalRun);
        res.cmdLinearDispatch(cmd, getWorkGroupCount(frame.traversalPersistentThreads, TRAVERSAL_RUN_WORKGROUP));

      if(m_config.useSeparateGroups)
      {
        memBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_INDIRECT_COMMAND_READ_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, 0, 1, &memBarrier, 0, nullptr, 0, nullptr);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelines.computeTraversalGroups);
        vkCmdDispatchIndirect(cmd, m_sceneBuildBuffer.buffer, offsetof(shaderio::SceneBuilding, indirectDispatchGroups));
      }


      memBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
      memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_DEPENDENCY_BY_REGION_BIT, 1, &memBarrier, 0, nullptr, 0, nullptr);

      uint32_t buildSetupID = BUILD_SETUP_DRAW;

      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelines.computeBuildSetup);
      vkCmdPushConstants(cmd, m_pipelineLayout, m_stageFlags, 0, sizeof(uint32_t), &buildSetupID);

      vkCmdDispatch(cmd, 1, 1, 1);


      memBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
      memBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
      vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                           VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
                           VK_DEPENDENCY_BY_REGION_BIT, 1, &memBarrier, 0, nullptr, 0, nullptr);
    }

    if(pass == lastPass && rscene.useStreaming)
    {

      rscene.sceneStreaming.cmdPostTraversal(cmd, true, profiler);
    }
    else if(rscene.useStreaming)
    {

      auto timerSection = profiler.cmdFrameSection(cmd, "Stream Post Traversal");
    }
    {

       auto timerSection = profiler.cmdFrameSection(cmd, "Draw");

      VkAttachmentLoadOp op = pass == 1 ? VK_ATTACHMENT_LOAD_OP_LOAD : (m_config.useShading ? VK_ATTACHMENT_LOAD_OP_DONT_CARE : VK_ATTACHMENT_LOAD_OP_CLEAR);

      res.cmdBeginRendering(cmd, false, op, op);
      if(pass == 0 && m_config.useShading)
      {

        writeBackgroundSky(cmd);
      }

      {

        auto timerSection = profiler.cmdFrameSection(cmd, "HW-Raster");

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, m_dsetPack.getSetPtr(), 0, nullptr);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines.graphicsMesh);

        if(m_config.useEXTmeshShader)
        {
          vkCmdDrawMeshTasksIndirectEXT(cmd, m_sceneBuildBuffer.buffer, offsetof(shaderio::SceneBuilding, indirectDrawClustersEXT), 1, 0);
        }
        else
        {
          vkCmdDrawMeshTasksIndirectNV(cmd, m_sceneBuildBuffer.buffer, offsetof(shaderio::SceneBuilding, indirectDrawClustersNV), 1, 0);
        }
      }

      if(frame.showClusterBboxes)
      {

        renderClusterBboxes(cmd, m_sceneBuildBuffer);
      }

      if(pass == lastPass && frame.showInstanceBboxes)
      {

        renderInstanceBboxes(cmd);
      }

      vkCmdEndRendering(cmd);
    }


    if(!frame.freezeCulling)
    {
      if(twoPassActive)
      {

        res.cmdBuildHiz(cmd, frame, profiler, pass ^ 1);
      }
      else
      {

        res.cmdBuildHiz(cmd, frame, profiler, 0);
      }
    }
  }

  if(passCount > 1)
  {

    profiler.getProfilerTimeline()->frameAccumulationSplit();
  }


  if(rscene.useStreaming)
  {

    rscene.sceneStreaming.cmdEndFrame(cmd, res.m_queueStates.primary, profiler);
  }


  m_resourceReservedUsage.geometryMemBytes = rscene.getGeometrySize(true);
  m_resourceActualUsage                    = m_resourceReservedUsage;

  m_resourceActualUsage.geometryMemBytes   = rscene.getGeometrySize(false);
  m_frameIndex++;
}


// 鍑芥暟锛歊endererRasterClustersLod::updatedFrameBuffer銆傚綍鍒舵垨鎵ц娓叉煋鐩稿叧宸ヤ綔锛屾妸鍑嗗濂界殑鏁版嵁鎻愪氦鍒板綋鍓嶆覆鏌撻樁娈点€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氭覆鏌撳嚱鏁伴€氬父澶勪簬甯х骇鍏抽敭璺緞锛屽繀椤诲皧閲嶅墠搴忚绠楅樁娈靛啓鍑虹殑璁℃暟銆佸湴鍧€鍜屽悓姝ュ睆闅溿€?void RendererRasterClustersLod::updatedFrameBuffer(Resources& res, RenderScene& rscene)
void RendererRasterClustersLod::updatedFrameBuffer(Resources& res, RenderScene& rscene)
{

  vkDeviceWaitIdle(res.m_device);


  VkWriteDescriptorSet writes[2];

  writes[0] = m_dsetPack.makeWrite(BINDINGS_HIZ_TEX, 0, 0);
  writes[0].pImageInfo = &res.m_hizUpdate[0].farImageInfo;

  if(m_config.useCulling && m_config.useTwoPassCulling)
  {

    writes[1]            = m_dsetPack.makeWrite(BINDINGS_HIZ_TEX, 0, 1);
    writes[1].pImageInfo = &res.m_hizUpdate[1].farImageInfo;
  }


  vkUpdateDescriptorSets(res.m_device, m_config.useCulling && m_config.useTwoPassCulling ? 2 : 1, writes, 0, nullptr);

  Renderer::updatedFrameBuffer(res, rscene);
}


// 鍑芥暟锛歊endererRasterClustersLod::deinit銆傞噴鏀炬垨鍥炴敹鍓嶉潰鍒濆鍖栫殑璧勬簮锛屼繚鎸佺敓鍛藉懆鏈熸垚瀵圭鐞嗐€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氶噴鏀鹃『搴忚閬靛畧璧勬簮渚濊禆鍏崇郴锛岄伩鍏?GPU 浠嶅彲鑳借闂殑瀵硅薄琚彁鍓嶉攢姣併€?void RendererRasterClustersLod::deinit(Resources& res)
void RendererRasterClustersLod::deinit(Resources& res)
{


  res.destroyPipelines(m_pipelines);


  vkDestroyPipelineLayout(res.m_device, m_pipelineLayout, nullptr);


  m_dsetPack.deinit();


  res.m_allocator.destroyBuffer(m_sceneDataBuffer);

  res.m_allocator.destroyBuffer(m_assemblyNodeBuffer);

  res.m_allocator.destroyBuffer(m_assemblyStateBuffer);

  res.m_allocator.destroyBuffer(m_sceneBuildBuffer);

  res.m_allocator.destroyBuffer(m_sceneTraversalBuffer);

  deinitBasics(res);
}


// 鍑芥暟锛歮akeRendererRasterClustersLod銆傚綍鍒舵垨鎵ц娓叉煋鐩稿叧宸ヤ綔锛屾妸鍑嗗濂界殑鏁版嵁鎻愪氦鍒板綋鍓嶆覆鏌撻樁娈点€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氭覆鏌撳嚱鏁伴€氬父澶勪簬甯х骇鍏抽敭璺緞锛屽繀椤诲皧閲嶅墠搴忚绠楅樁娈靛啓鍑虹殑璁℃暟銆佸湴鍧€鍜屽悓姝ュ睆闅溿€?std::unique_ptr<Renderer> makeRendererRasterClustersLod()
std::unique_ptr<Renderer> makeRendererRasterClustersLod()
{
  return std::make_unique<RendererRasterClustersLod>();
}
}
