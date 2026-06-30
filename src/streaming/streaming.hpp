//==============================================================================
// 鏂囦欢锛歴rc/streaming/streaming.hpp
// 妯″潡瀹氫綅锛歋ceneStreaming 楂樺眰鎺ュ彛澹版槑锛岀粺涓€绠＄悊涓€涓満鏅湪 娴佸紡鍔犺浇 妯″紡涓嬬殑 GPU 鏁版嵁鍜屾瘡甯ц皟搴﹀叆鍙ｃ€?// 鏁版嵁娴侊細Renderer 鍦ㄩ亶鍘嗗墠鍚庤皟鐢?begin/pre/post/end锛汼ceneStreaming 鍦ㄨ繖浜涢樁娈靛鐞嗚姹傘€佷笂浼犮€佸湴鍧€鏇存柊鍜岀粺璁°€?// 鏂规硶璇存槑锛氶珮灞傛帴鍙ｆ妸鎸夐渶椹荤暀鏈哄埗灏佽鎴?renderer 鍙彃鎷旂粍浠讹紝浣块鍔犺浇鍜屾祦寮忓姞杞藉叡浜悓涓€閬嶅巻/娓叉煋閫昏緫銆?// 姝ｇ‘鎬х害鏉燂細姣忓抚璋冪敤椤哄簭蹇呴』鍥哄畾锛況eset 瑕佸悓鏃舵竻绌?resident 鐘舵€佸拰 Geometry 鍦板潃锛沝escriptor 鏇存柊蹇呴』瑕嗙洊 traversal 鍜?render 闇€瑕佺殑 缂撳啿銆?// 娉ㄩ噴椋庢牸锛氫娇鐢ㄤ腑鏂囪В閲?CPU 渚ц涔夛紱淇濈暀蹇呰鐨?API銆佺被鍨嬪悕鍜屾暟瀛︾缉鍐欎互渚挎绱€?//==============================================================================
#pragma once


// 渚濊禆璇存槑锛氬紩鍏ユ湰缂栬瘧鍗曞厓闇€瑕佺殑澶栭儴搴撱€侀」鐩ā鍧楀拰鍏变韩鐫€鑹插櫒甯冨眬銆?// 渚濊禆椤哄簭閫氬父鍙嶆槧鎶借薄灞傛锛氬厛澶栭儴搴擄紝鍐嶉」鐩ā鍧楋紝鏈€鍚庝笌 GPU 鍏变韩鐨勬帴鍙ｅ畾涔夈€?#include "streamutils.hpp"
#include "streamutils.hpp"


// 鍛藉悕绌洪棿璇存槑锛氶檺鍒剁鍙峰彲瑙佽寖鍥达紝骞惰〃鏄庤繖浜涚被鍨嬪拰鍑芥暟灞炰簬鍚屼竴鍔熻兘鍩熴€?// 璇ヨ竟鐣屾湁鍔╀簬鍖哄垎搴旂敤灞傘€佹覆鏌撳眰銆佸満鏅眰鍜岀畻娉曞眰鐨勮亴璐ｃ€?namespace lodclusters {
namespace lodclusters {


// 绫诲瀷锛歋ceneStreaming銆傚皝瑁呮湰妯″潡鐨勯暱鏈熺姸鎬併€佽祫婧愭墍鏈夋潈鍜屽澶栨搷浣滄帴鍙ｃ€?// 璁捐鎰忓浘锛氶€氳繃鎴愬憳鍑芥暟闆嗕腑缁存姢鐘舵€佽浆绉伙紝閬垮厤璋冪敤鏂圭洿鎺ユ嫾鎺ュ簳灞傝祫婧愮敓鍛藉懆鏈熴€?// 浣跨敤绾︽潫锛氬疄渚嬪垵濮嬪寲銆佹瘡甯т娇鐢ㄥ拰閲婃斁搴旈伒瀹堝０鏄庨『搴忓搴旂殑渚濊禆鍏崇郴銆?class SceneStreaming
class SceneStreaming
{
public:


  // 鍑芥暟锛歩nit銆傚垵濮嬪寲鏈ā鍧楁墍闇€鐘舵€併€佽祫婧愭垨 GPU 渚х粦瀹氥€?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氬垵濮嬪寲杩囩▼寤虹珛鍚庣画闃舵鍋囧畾瀛樺湪鐨勪笉鍙橀噺锛屼緥濡傚彞鏌勬湁鏁堛€佺紦鍐插ぇ灏忚冻澶熴€佹弿杩扮宸茬粦瀹氥€?  bool init(Resources* res, const Scene* scene, const StreamingConfig& config);
  bool init(Resources* res, const Scene* scene, const StreamingConfig& config);


  // 鍑芥暟锛歞einit銆傞噴鏀炬垨鍥炴敹鍓嶉潰鍒濆鍖栫殑璧勬簮锛屼繚鎸佺敓鍛藉懆鏈熸垚瀵圭鐞嗐€?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氶噴鏀鹃『搴忚閬靛畧璧勬簮渚濊禆鍏崇郴锛岄伩鍏?GPU 浠嶅彲鑳借闂殑瀵硅薄琚彁鍓嶉攢姣併€?  void deinit();
  void deinit();


  // 鍑芥暟锛歳eset銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  void reset();
  void reset();


  // 鍑芥暟锛歳eloadShaders銆備粠鏂囦欢銆佺紦瀛樸€丟PU 缂撳啿鎴栧叡浜竷灞€涓鍙栨暟鎹苟杞崲涓烘湰妯″潡鏍煎紡銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍙栬矾寰勯渶瑕佹牎楠岃緭鍏ュ悎娉曟€э紝骞舵妸澶栭儴鏍煎紡鐨勪笉纭畾鎬ц浆鍖栦负鍐呴儴纭畾甯冨眬銆?  bool reloadShaders()
  bool reloadShaders()
  {

    deinitShadersAndPipelines();
    return initShadersAndPipelines();
  }


  // 缁撴瀯锛欶rameSettings銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?  // 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?  // 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?  struct FrameSettings
  struct FrameSettings
  {
    uint32_t ageThreshold = 16;
  };
  void cmdBeginFrame(VkCommandBuffer         cmd,
                     QueueState&             cmdQueueState,
                     QueueState&             asyncQueueState,
                     const FrameSettings&    settings,
                     nvvk::ProfilerGpuTimer& profiler);


  // 鍑芥暟锛歝mdPreTraversal銆傚悜鍛戒护缂撳啿褰曞埗 GPU 鎿嶄綔锛屽苟渚濊禆澶栧眰璋冪敤鑰呭畨鎺掓彁浜や笌鍚屾銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳绫诲嚱鏁板彧鎻忚堪鍛戒护搴忓垪锛屼笉搴斿亣璁惧懡浠ゅ凡缁忕珛鍗虫墽琛屻€?  void cmdPreTraversal(VkCommandBuffer cmd, nvvk::ProfilerGpuTimer& profiler);
  void cmdPreTraversal(VkCommandBuffer cmd, nvvk::ProfilerGpuTimer& profiler);


  // 鍑芥暟锛歝mdPostTraversal銆傚悜鍛戒护缂撳啿褰曞埗 GPU 鎿嶄綔锛屽苟渚濊禆澶栧眰璋冪敤鑰呭畨鎺掓彁浜や笌鍚屾銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳绫诲嚱鏁板彧鎻忚堪鍛戒护搴忓垪锛屼笉搴斿亣璁惧懡浠ゅ凡缁忕珛鍗虫墽琛屻€?  void cmdPostTraversal(VkCommandBuffer cmd, bool runAgeFilter, nvvk::ProfilerGpuTimer& profiler);
  void cmdPostTraversal(VkCommandBuffer cmd, bool runAgeFilter, nvvk::ProfilerGpuTimer& profiler);


  // 鍑芥暟锛歝mdEndFrame銆傚悜鍛戒护缂撳啿褰曞埗 GPU 鎿嶄綔锛屽苟渚濊禆澶栧眰璋冪敤鑰呭畨鎺掓彁浜や笌鍚屾銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳绫诲嚱鏁板彧鎻忚堪鍛戒护搴忓垪锛屼笉搴斿亣璁惧懡浠ゅ凡缁忕珛鍗虫墽琛屻€?  void cmdEndFrame(VkCommandBuffer cmd, QueueState& cmdQueueState, nvvk::ProfilerGpuTimer& profiler);
  void cmdEndFrame(VkCommandBuffer cmd, QueueState& cmdQueueState, nvvk::ProfilerGpuTimer& profiler);


  // 鍑芥暟锛歡etStats銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  void getStats(StreamingStats& stats) const;
  void getStats(StreamingStats& stats) const;
  const nvvk::BufferTyped<shaderio::Geometry>& getShaderGeometriesBuffer() const { return m_shaderGeometriesBuffer; }
  const nvvk::Buffer&                          getShaderStreamingBuffer() const { return m_shaderBuffer; }
  const shaderio::SceneStreaming&              getShaderStreamingData() const { return m_shaderData; }
  const StreamingConfig&                       getStreamingConfig() const { return m_config; }


  // 鍑芥暟锛歡etGeometrySize銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  size_t getGeometrySize(bool reserved) const;
  size_t getGeometrySize(bool reserved) const;
  size_t getOperationsSize() const { return m_operationsSize; }


  // 鍑芥暟锛歶pdateBindings銆傛牴鎹渶鏂扮姸鎬佸埛鏂扮紦瀛樻暟鎹€丟PU 鍦板潃銆佹弿杩扮鎴栫粺璁′俊鎭€?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氭洿鏂板嚱鏁拌礋璐ｆ妸鈥滄棫鐘舵€佲€濇帹杩涘埌鈥滃綋鍓嶇姸鎬佲€濓紝鍥犳瑕侀伩鍏嶉儴鍒嗘洿鏂伴€犳垚 CPU/GPU 瑙嗗浘涓嶄竴鑷淬€?  void updateBindings(const nvvk::Buffer& sceneBuildingBuffer);
  void updateBindings(const nvvk::Buffer& sceneBuildingBuffer);
#ifndef NDEBUG
  static const int32_t s_defaultDebugFrameLimit = -1;
#else
  static const int32_t s_defaultDebugFrameLimit = -1;
#endif
  int32_t m_debugFrameLimit = s_defaultDebugFrameLimit;

private:
  Resources*   m_resources = nullptr;
  const Scene* m_scene     = nullptr;

  StreamingConfig m_config;
  size_t          m_persistentGeometrySize;
  size_t          m_operationsSize;
  size_t          m_peakGeometrySize = 0;
  uint32_t        m_peakFrameIndex   = ~0;
  uint32_t        m_lastUpdateIndex;
  uint32_t        m_frameIndex;
  StreamingStats  m_stats;


  // 缁撴瀯锛歅ersistentGeometry銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?  // 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?  // 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?  struct PersistentGeometry
  struct PersistentGeometry
  {
    nvvk::BufferTyped<shaderio::Node>     nodes;
    nvvk::BufferTyped<shaderio::BBox>     nodeBboxes;
    nvvk::BufferTyped<shaderio::LodLevel> lodLevels;
    nvvk::BufferTyped<uint64_t>           groupAddresses;
    nvvk::Buffer                          lowDetailGroupsData;
    uint32_t                              lodLevelsCount                                = 0;
    uint32_t                              lodLoadedGroupsCount[SHADERIO_MAX_LOD_LEVELS] = {};
    uint32_t                              lodGroupsCount[SHADERIO_MAX_LOD_LEVELS]       = {};
  };

  std::vector<PersistentGeometry>       m_persistentGeometries;
  std::vector<shaderio::Geometry>       m_shaderGeometries;
  nvvk::BufferTyped<shaderio::Geometry> m_shaderGeometriesBuffer;


  // 鍑芥暟锛歩nitGeometries銆傚垵濮嬪寲鏈ā鍧楁墍闇€鐘舵€併€佽祫婧愭垨 GPU 渚х粦瀹氥€?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氬垵濮嬪寲杩囩▼寤虹珛鍚庣画闃舵鍋囧畾瀛樺湪鐨勪笉鍙橀噺锛屼緥濡傚彞鏌勬湁鏁堛€佺紦鍐插ぇ灏忚冻澶熴€佹弿杩扮宸茬粦瀹氥€?  void initGeometries(Resources& res, const Scene* scene);
  void initGeometries(Resources& res, const Scene* scene);


  // 鍑芥暟锛歳esetGeometryGroupAddresses銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  void resetGeometryGroupAddresses(Resources::BatchedUploader& uploader);
  void resetGeometryGroupAddresses(Resources::BatchedUploader& uploader);
  shaderio::SceneStreaming m_shaderData;
  nvvk::Buffer             m_shaderBuffer;
  StreamingTaskQueue m_requestsTaskQueue;
  StreamingTaskQueue m_storageTaskQueue;
  StreamingTaskQueue m_updatesTaskQueue;
  StreamingRequests m_requests;
  StreamingResident m_resident;
  StreamingStorage m_storage;
  StreamingUpdates m_updates;
  uint32_t handleCompletedRequest(VkCommandBuffer      cmd,
                                  QueueState&          cmdQueueState,
                                  QueueState&          asyncQueueState,
                                  const FrameSettings& settings,
                                  uint32_t             popRequestIndex);


  // 缁撴瀯锛歋haders銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?  // 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?  // 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?  struct Shaders
  struct Shaders
  {
    shaderc::SpvCompilationResult computeAgeFilterGroups;
    shaderc::SpvCompilationResult computeUpdateSceneRaster;
  };


  // 缁撴瀯锛歅ipelines銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?  // 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?  // 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?  struct Pipelines
  struct Pipelines
  {
    VkPipeline computeAgeFilterGroups   = nullptr;
    VkPipeline computeUpdateSceneRaster = nullptr;
  };

  Shaders              m_shaders;
  Pipelines            m_pipelines;
  VkPipelineLayout     m_pipelineLayout{};
  nvvk::DescriptorPack m_dsetPack;


  // 鍑芥暟锛歩nitShadersAndPipelines銆傚垵濮嬪寲鏈ā鍧楁墍闇€鐘舵€併€佽祫婧愭垨 GPU 渚х粦瀹氥€?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氬垵濮嬪寲杩囩▼寤虹珛鍚庣画闃舵鍋囧畾瀛樺湪鐨勪笉鍙橀噺锛屼緥濡傚彞鏌勬湁鏁堛€佺紦鍐插ぇ灏忚冻澶熴€佹弿杩扮宸茬粦瀹氥€?  bool initShadersAndPipelines();
  bool initShadersAndPipelines();


  // 鍑芥暟锛歞einitShadersAndPipelines銆傞噴鏀炬垨鍥炴敹鍓嶉潰鍒濆鍖栫殑璧勬簮锛屼繚鎸佺敓鍛藉懆鏈熸垚瀵圭鐞嗐€?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氶噴鏀鹃『搴忚閬靛畧璧勬簮渚濊禆鍏崇郴锛岄伩鍏?GPU 浠嶅彲鑳借闂殑瀵硅薄琚彁鍓嶉攢姣併€?  void deinitShadersAndPipelines();
  void deinitShadersAndPipelines();
};
}
