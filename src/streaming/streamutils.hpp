//==============================================================================
// 鏂囦欢锛歴rc/streaming/streamutils.hpp
// 妯″潡瀹氫綅锛氭祦寮忓姞杞藉熀纭€缁撴瀯澹版槑锛屽畾涔夎姹傞槦鍒椼€侀┗鐣欒〃銆佸垎閰嶅櫒銆佸瓨鍌ㄣ€佹洿鏂颁换鍔″拰寮傛浠诲姟闃熷垪銆?// 鏁版嵁娴侊細GPU 鍐欏嚭 load/unload request锛汣PU 澶勭悊鍚庨€氳繃杩欎簺缁撴瀯鎶婄粨鏋滀笂浼犲洖 GPU 鍦板潃琛ㄥ拰 resident 鐘舵€併€?// 鏂规硶璇存槑锛氭祦寮忓姞杞芥妸鍑犱綍椹荤暀瑙嗕负鍙楅檺璧勬簮鍒嗛厤闂锛岄渶瑕佸悓鏃剁鐞嗚姹傛帓搴忋€佺┖闂村垎閰嶃€佷紶杈撳甫瀹藉拰鍙鎬т繚娲汇€?// 姝ｇ‘鎬х害鏉燂細residentID 蹇呴』绋冲畾鏄犲皠鍒?缁?鐘舵€侊紱浠诲姟闃熷垪绱㈠紩涓嶈兘閲嶅閲婃斁锛沘llocator 鐨?free gap 涓?storage 鐘舵€佸繀椤讳竴鑷淬€?// 娉ㄩ噴椋庢牸锛氫娇鐢ㄤ腑鏂囪В閲?CPU 渚ц涔夛紱淇濈暀蹇呰鐨?API銆佺被鍨嬪悕鍜屾暟瀛︾缉鍐欎互渚挎绱€?//==============================================================================
#pragma once


// 渚濊禆璇存槑锛氬紩鍏ユ湰缂栬瘧鍗曞厓闇€瑕佺殑澶栭儴搴撱€侀」鐩ā鍧楀拰鍏变韩鐫€鑹插櫒甯冨眬銆?// 渚濊禆椤哄簭閫氬父鍙嶆槧鎶借薄灞傛锛氬厛澶栭儴搴擄紝鍐嶉」鐩ā鍧楋紝鏈€鍚庝笌 GPU 鍏变韩鐨勬帴鍙ｅ畾涔夈€?#include <queue>
#include <queue>
#include <nvutils/logger.hpp>
#include <nvutils/id_pool.hpp>
#include <nvvk/buffer_suballocator.hpp>
#include <nvvk/command_pools.hpp>
#include "scene.hpp"
#include "resources.hpp"
#include "shaderio_streaming.h"


// 鍛藉悕绌洪棿璇存槑锛氶檺鍒剁鍙峰彲瑙佽寖鍥达紝骞惰〃鏄庤繖浜涚被鍨嬪拰鍑芥暟灞炰簬鍚屼竴鍔熻兘鍩熴€?// 璇ヨ竟鐣屾湁鍔╀簬鍖哄垎搴旂敤灞傘€佹覆鏌撳眰銆佸満鏅眰鍜岀畻娉曞眰鐨勮亴璐ｃ€?namespace lodclusters {
namespace lodclusters {

static const uint32_t STREAMING_MAX_ACTIVE_TASKS = 3;
static const uint32_t INVALID_TASK_INDEX         = ~0;


// 缁撴瀯锛歋treamingConfig銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?// 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?// 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?struct StreamingConfig
struct StreamingConfig
{
  bool useAsyncTransfer           = false;
  bool useDecoupledAsyncTransfer  = false;

  uint32_t maxPerFrameLoadRequests   = 128;
  uint32_t maxPerFrameUnloadRequests = 1024;

  uint32_t maxGroups   = 1 << 16;
  uint32_t maxClusters = 0;

  size_t maxTransferMegaBytes    = 32;
  size_t maxGeometryMegaBytes    = 1024 * 2;
};


// 缁撴瀯锛歋treamingStats銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?// 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?// 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?struct StreamingStats
struct StreamingStats
{
  uint32_t residentGroups   = 0;
  uint32_t residentClusters = 0;
  uint32_t maxGroups        = 0;
  uint32_t maxClusters      = 0;

  uint32_t persistentGroups    = 0;
  uint32_t persistentClusters  = 0;
  uint64_t persistentDataBytes = 0;

  uint64_t maxDataBytes      = 0;
  uint64_t reservedDataBytes = 0;
  uint64_t usedDataBytes     = 0;

  uint32_t maxSizedLeft      = 0;
  uint32_t maxSizedReserved  = 0;

  uint64_t maxTransferBytes     = 0;
  uint64_t transferBytes        = 0;
  uint32_t transferCount        = 0;
  uint32_t loadCount            = 0;
  uint32_t unloadCount          = 0;
  uint32_t uncompletedLoadCount = 0;
  uint32_t maxLoadCount         = 0;
  uint32_t maxUnloadCount       = 0;

  uint32_t couldNotAllocateGroup = 0;
  uint32_t couldNotTransfer      = 0;
  uint32_t couldNotStore         = 0;
};

union GeometryGroup
{
  struct
  {
    uint32_t geometryID;
    uint32_t groupID;
  };
  uint64_t key;
};


// 绫诲瀷锛歋treamingRequests銆傚皝瑁呮湰妯″潡鐨勯暱鏈熺姸鎬併€佽祫婧愭墍鏈夋潈鍜屽澶栨搷浣滄帴鍙ｃ€?// 璁捐鎰忓浘锛氶€氳繃鎴愬憳鍑芥暟闆嗕腑缁存姢鐘舵€佽浆绉伙紝閬垮厤璋冪敤鏂圭洿鎺ユ嫾鎺ュ簳灞傝祫婧愮敓鍛藉懆鏈熴€?// 浣跨敤绾︽潫锛氬疄渚嬪垵濮嬪寲銆佹瘡甯т娇鐢ㄥ拰閲婃斁搴旈伒瀹堝０鏄庨『搴忓搴旂殑渚濊禆鍏崇郴銆?class StreamingRequests
class StreamingRequests
{
public:


  // 缁撴瀯锛歍askInfo銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?  // 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?  // 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?  struct TaskInfo
  struct TaskInfo
  {
    const shaderio::StreamingRequest* shaderData;
    const GeometryGroup*              loadGeometryGroups;
    const GeometryGroup*              unloadGeometryGroups;
  };


  // 鍑芥暟锛歩nit銆傚垵濮嬪寲鏈ā鍧楁墍闇€鐘舵€併€佽祫婧愭垨 GPU 渚х粦瀹氥€?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氬垵濮嬪寲杩囩▼寤虹珛鍚庣画闃舵鍋囧畾瀛樺湪鐨勪笉鍙橀噺锛屼緥濡傚彞鏌勬湁鏁堛€佺紦鍐插ぇ灏忚冻澶熴€佹弿杩扮宸茬粦瀹氥€?  void init(Resources& res, const StreamingConfig& config, uint32_t groupCountAlignment, uint32_t clusterCountAlignment);
  void init(Resources& res, const StreamingConfig& config, uint32_t groupCountAlignment, uint32_t clusterCountAlignment);


  // 鍑芥暟锛歞einit銆傞噴鏀炬垨鍥炴敹鍓嶉潰鍒濆鍖栫殑璧勬簮锛屼繚鎸佺敓鍛藉懆鏈熸垚瀵圭鐞嗐€?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氶噴鏀鹃『搴忚閬靛畧璧勬簮渚濊禆鍏崇郴锛岄伩鍏?GPU 浠嶅彲鑳借闂殑瀵硅薄琚彁鍓嶉攢姣併€?  void deinit(Resources& res);
  void deinit(Resources& res);


  // 鍑芥暟锛歡etOperationsSize銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  size_t getOperationsSize() const;
  size_t getOperationsSize() const;


  // 鍑芥暟锛歛pplyTask銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  void applyTask(shaderio::StreamingRequest& shaderData, uint32_t taskIndex, uint32_t frameIndex);
  void applyTask(shaderio::StreamingRequest& shaderData, uint32_t taskIndex, uint32_t frameIndex);


  // 鍑芥暟锛歝mdRunTask銆傚悜鍛戒护缂撳啿褰曞埗 GPU 鎿嶄綔锛屽苟渚濊禆澶栧眰璋冪敤鑰呭畨鎺掓彁浜や笌鍚屾銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳绫诲嚱鏁板彧鎻忚堪鍛戒护搴忓垪锛屼笉搴斿亣璁惧懡浠ゅ凡缁忕珛鍗虫墽琛屻€?  void cmdRunTask(VkCommandBuffer cmd, const shaderio::StreamingRequest& shaderData, VkBuffer srcBuffer, size_t srcBufferOffset);
  void cmdRunTask(VkCommandBuffer cmd, const shaderio::StreamingRequest& shaderData, VkBuffer srcBuffer, size_t srcBufferOffset);


  const TaskInfo& getCompletedTask(uint32_t taskIndex) { return m_taskInfos[taskIndex]; }

private:
  nvvk::Buffer m_requestBuffer;
  nvvk::Buffer m_requestHostBuffer;

  uint64_t m_requestSize;
  uint64_t m_shaderDataOffset;

  shaderio::StreamingRequest m_shaderData;
  TaskInfo                   m_taskInfos[STREAMING_MAX_ACTIVE_TASKS];
};


// 绫诲瀷锛歋treamingResident銆傚皝瑁呮湰妯″潡鐨勯暱鏈熺姸鎬併€佽祫婧愭墍鏈夋潈鍜屽澶栨搷浣滄帴鍙ｃ€?// 璁捐鎰忓浘锛氶€氳繃鎴愬憳鍑芥暟闆嗕腑缁存姢鐘舵€佽浆绉伙紝閬垮厤璋冪敤鏂圭洿鎺ユ嫾鎺ュ簳灞傝祫婧愮敓鍛藉懆鏈熴€?// 浣跨敤绾︽潫锛氬疄渚嬪垵濮嬪寲銆佹瘡甯т娇鐢ㄥ拰閲婃斁搴旈伒瀹堝０鏄庨『搴忓搴旂殑渚濊禆鍏崇郴銆?class StreamingResident
class StreamingResident
{
public:
  static const uint32_t INVALID_GROUP = ~0;


  // 缁撴瀯锛欸roup銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?  // 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?  // 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?  struct Group
  struct Group
  {
    GeometryGroup             geometryGroup;
    uint32_t                  activeIndex;
    uint32_t                  groupResidentID;
    uint32_t                  clusterResidentID;
    uint16_t                  clusterCount;
    uint16_t                  lodLevel;
    uint64_t                  deviceAddress;
    nvvk::BufferSubAllocation storageHandle;
  };


  // 鍑芥暟锛歩nit銆傚垵濮嬪寲鏈ā鍧楁墍闇€鐘舵€併€佽祫婧愭垨 GPU 渚х粦瀹氥€?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氬垵濮嬪寲杩囩▼寤虹珛鍚庣画闃舵鍋囧畾瀛樺湪鐨勪笉鍙橀噺锛屼緥濡傚彞鏌勬湁鏁堛€佺紦鍐插ぇ灏忚冻澶熴€佹弿杩扮宸茬粦瀹氥€?  void init(Resources& res, const StreamingConfig& config, uint32_t groupCountAlignment, uint32_t clusterCountAlignment);
  void init(Resources& res, const StreamingConfig& config, uint32_t groupCountAlignment, uint32_t clusterCountAlignment);


  // 鍑芥暟锛歞einit銆傞噴鏀炬垨鍥炴敹鍓嶉潰鍒濆鍖栫殑璧勬簮锛屼繚鎸佺敓鍛藉懆鏈熸垚瀵圭鐞嗐€?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氶噴鏀鹃『搴忚閬靛畧璧勬簮渚濊禆鍏崇郴锛岄伩鍏?GPU 浠嶅彲鑳借闂殑瀵硅薄琚彁鍓嶉攢姣併€?  void         deinit(Resources& res);
  void         deinit(Resources& res);


  // 鍑芥暟锛歳eset銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  void         reset(shaderio::StreamingResident& shaderData);
  void         reset(shaderio::StreamingResident& shaderData);


  // 鍑芥暟锛歡etOperationsSize銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  size_t getOperationsSize() const;
  size_t getOperationsSize() const;


  // 鍑芥暟锛歡etStats銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  void   getStats(StreamingStats& stats) const;
  void   getStats(StreamingStats& stats) const;


  // 鍑芥暟锛歶ploadInitialState銆傚垵濮嬪寲鏈ā鍧楁墍闇€鐘舵€併€佽祫婧愭垨 GPU 渚х粦瀹氥€?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氬垵濮嬪寲杩囩▼寤虹珛鍚庣画闃舵鍋囧畾瀛樺湪鐨勪笉鍙橀噺锛屼緥濡傚彞鏌勬湁鏁堛€佺紦鍐插ぇ灏忚冻澶熴€佹弿杩扮宸茬粦瀹氥€?  void                uploadInitialState(Resources::BatchedUploader& uploader, shaderio::StreamingResident& shaderData);
  void                uploadInitialState(Resources::BatchedUploader& uploader, shaderio::StreamingResident& shaderData);


  // 鍑芥暟锛歠indGroup銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  const StreamingResident::Group* findGroup(GeometryGroup geometryGroup) const;
  const StreamingResident::Group* findGroup(GeometryGroup geometryGroup) const;
  const StreamingResident::Group& getGroup(uint32_t groupResidentID) const { return m_groups[groupResidentID]; }


  // 鍑芥暟锛歡etLoadActiveGroupsOffset銆備粠鏂囦欢銆佺紦瀛樸€丟PU 缂撳啿鎴栧叡浜竷灞€涓鍙栨暟鎹苟杞崲涓烘湰妯″潡鏍煎紡銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍙栬矾寰勯渶瑕佹牎楠岃緭鍏ュ悎娉曟€э紝骞舵妸澶栭儴鏍煎紡鐨勪笉纭畾鎬ц浆鍖栦负鍐呴儴纭畾甯冨眬銆?  uint32_t getLoadActiveGroupsOffset() const;
  uint32_t getLoadActiveGroupsOffset() const;


  // 鍑芥暟锛歡etLoadActiveClustersOffset銆備粠鏂囦欢銆佺紦瀛樸€丟PU 缂撳啿鎴栧叡浜竷灞€涓鍙栨暟鎹苟杞崲涓烘湰妯″潡鏍煎紡銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍙栬矾寰勯渶瑕佹牎楠岃緭鍏ュ悎娉曟€э紝骞舵妸澶栭儴鏍煎紡鐨勪笉纭畾鎬ц浆鍖栦负鍐呴儴纭畾甯冨眬銆?  uint32_t getLoadActiveClustersOffset() const;
  uint32_t getLoadActiveClustersOffset() const;


  // 鍑芥暟锛歝anAllocateGroup銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  bool                      canAllocateGroup(uint32_t numClusters) const;
  bool                      canAllocateGroup(uint32_t numClusters) const;


  // 鍑芥暟锛歛ddGroup銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  StreamingResident::Group* addGroup(GeometryGroup geometryGroup, uint32_t clusterCount);
  StreamingResident::Group* addGroup(GeometryGroup geometryGroup, uint32_t clusterCount);


  // 鍑芥暟锛歳emoveGroup銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  void                      removeGroup(uint32_t groupResidentID);
  void                      removeGroup(uint32_t groupResidentID);


  // 鍑芥暟锛歝mdUploadTask銆傚悜鍛戒护缂撳啿褰曞埗 GPU 鎿嶄綔锛屽苟渚濊禆澶栧眰璋冪敤鑰呭畨鎺掓彁浜や笌鍚屾銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳绫诲嚱鏁板彧鎻忚堪鍛戒护搴忓垪锛屼笉搴斿亣璁惧懡浠ゅ凡缁忕珛鍗虫墽琛屻€?  size_t cmdUploadTask(VkCommandBuffer cmd, uint32_t taskIndex);
  size_t cmdUploadTask(VkCommandBuffer cmd, uint32_t taskIndex);


  // 鍑芥暟锛歛pplyTask銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  void applyTask(shaderio::StreamingResident& shaderData, uint32_t taskIndex, uint32_t frameIndex);
  void applyTask(shaderio::StreamingResident& shaderData, uint32_t taskIndex, uint32_t frameIndex);


  // 鍑芥暟锛歝mdRunTask銆傚悜鍛戒护缂撳啿褰曞埗 GPU 鎿嶄綔锛屽苟渚濊禆澶栧眰璋冪敤鑰呭畨鎺掓彁浜や笌鍚屾銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳绫诲嚱鏁板彧鎻忚堪鍛戒护搴忓垪锛屼笉搴斿亣璁惧懡浠ゅ凡缁忕珛鍗虫墽琛屻€?  void cmdRunTask(VkCommandBuffer cmd, uint32_t taskIndex);
  void cmdRunTask(VkCommandBuffer cmd, uint32_t taskIndex);


private:


  // 缁撴瀯锛歎pdateRange銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?  // 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?  // 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?  struct UpdateRange
  struct UpdateRange
  {

    uint32_t lo = uint32_t(~0);
    uint32_t hi = 0;


    // 鍑芥暟锛歶pdate銆傛牴鎹渶鏂扮姸鎬佸埛鏂扮紦瀛樻暟鎹€丟PU 鍦板潃銆佹弿杩扮鎴栫粺璁′俊鎭€?    // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?    // 璁捐瑕佺偣锛氭洿鏂板嚱鏁拌礋璐ｆ妸鈥滄棫鐘舵€佲€濇帹杩涘埌鈥滃綋鍓嶇姸鎬佲€濓紝鍥犳瑕侀伩鍏嶉儴鍒嗘洿鏂伴€犳垚 CPU/GPU 瑙嗗浘涓嶄竴鑷淬€?    void update(uint32_t index)
    void update(uint32_t index)
    {

      lo = std::min(lo, index);

      hi = std::max(hi, index);
    }

    uint32_t count() const { return hi == 0 && lo == ~0 ? 0 : 1 + hi - lo; }
  };


  // 缁撴瀯锛歍askInfo銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?  // 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?  // 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?  struct TaskInfo
  struct TaskInfo
  {
    VkBufferCopy                region;
    shaderio::StreamingResident shaderData;
  };


  std::unordered_map<uint64_t, uint32_t> m_mapGeometryGroup2Residency;

  nvutils::IDPool m_groupAllocator;
  nvutils::IDPool m_clusterAllocator;

  uint32_t m_maxClusters;
  uint32_t m_maxGroups;

  std::vector<Group> m_groups;


  std::vector<uint32_t> m_activeGroupIndices;

  uint32_t m_lowDetailGroupsCount;
  uint32_t m_lowDetailClustersCount;
  uint32_t m_lowDetailMaxGroupClusters;

  uint32_t m_activeGroupsCount;
  uint32_t m_activeClustersCount;

  nvvk::Buffer m_residentBuffer;
  uint64_t     m_residentGroupsOffset;
  uint64_t     m_residentGroupIDsOffset;
  uint64_t     m_residentClustersOffset;
  uint64_t     m_residentActiveOffset;
  uint64_t     m_residentActiveUpdateOffset;

  shaderio::StreamingResident m_shaderData;

  nvvk::BufferTyped<uint32_t> m_residentActiveHostBuffer;
  UpdateRange                 m_groupIndicesUpdateRange;

  TaskInfo m_taskInfos[STREAMING_MAX_ACTIVE_TASKS];
};


// 绫诲瀷锛歋treamingUpdates銆傚皝瑁呮湰妯″潡鐨勯暱鏈熺姸鎬併€佽祫婧愭墍鏈夋潈鍜屽澶栨搷浣滄帴鍙ｃ€?// 璁捐鎰忓浘锛氶€氳繃鎴愬憳鍑芥暟闆嗕腑缁存姢鐘舵€佽浆绉伙紝閬垮厤璋冪敤鏂圭洿鎺ユ嫾鎺ュ簳灞傝祫婧愮敓鍛藉懆鏈熴€?// 浣跨敤绾︽潫锛氬疄渚嬪垵濮嬪寲銆佹瘡甯т娇鐢ㄥ拰閲婃斁搴旈伒瀹堝０鏄庨『搴忓搴旂殑渚濊禆鍏崇郴銆?class StreamingUpdates
class StreamingUpdates
{
public:


  // 缁撴瀯锛歍askInfo銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?  // 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?  // 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?  struct TaskInfo
  struct TaskInfo
  {
    uint32_t                          loadCount;
    uint32_t                          unloadCount;
    uint32_t                          newClusterCount;
    uint32_t                          loadActiveGroupsOffset;
    uint32_t                          loadActiveClustersOffset;
    shaderio::StreamingPatch*         loadPatches;
    shaderio::StreamingPatch*         unloadPatches;
    nvvk::BufferSubAllocation*        unloadHandles;
  };


  // 缁撴瀯锛歂ewInfo銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?  // 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?  // 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?  struct NewInfo
  struct NewInfo
  {
    uint32_t groups   = 0;
    uint32_t clusters = 0;
  };


  // 鍑芥暟锛歩nit銆傚垵濮嬪寲鏈ā鍧楁墍闇€鐘舵€併€佽祫婧愭垨 GPU 渚х粦瀹氥€?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氬垵濮嬪寲杩囩▼寤虹珛鍚庣画闃舵鍋囧畾瀛樺湪鐨勪笉鍙橀噺锛屼緥濡傚彞鏌勬湁鏁堛€佺紦鍐插ぇ灏忚冻澶熴€佹弿杩扮宸茬粦瀹氥€?  void init(Resources& res, const StreamingConfig& config, uint32_t groupCountAlignment, uint32_t clusterCountAlignment);
  void init(Resources& res, const StreamingConfig& config, uint32_t groupCountAlignment, uint32_t clusterCountAlignment);


  // 鍑芥暟锛歞einit銆傞噴鏀炬垨鍥炴敹鍓嶉潰鍒濆鍖栫殑璧勬簮锛屼繚鎸佺敓鍛藉懆鏈熸垚瀵圭鐞嗐€?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氶噴鏀鹃『搴忚閬靛畧璧勬簮渚濊禆鍏崇郴锛岄伩鍏?GPU 浠嶅彲鑳借闂殑瀵硅薄琚彁鍓嶉攢姣併€?  void deinit(Resources& res);
  void deinit(Resources& res);


  // 鍑芥暟锛歡etOperationsSize銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  size_t   getOperationsSize() const;
  size_t   getOperationsSize() const;


  // 鍑芥暟锛歳eset銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  void reset();
  void reset();


  // 鍑芥暟锛歡etFutureNew銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  NewInfo getFutureNew(uint64_t frameIndex) const
  NewInfo getFutureNew(uint64_t frameIndex) const
  {


    NewInfo info = m_pendingNew;


    for(uint32_t i = 0; i < STREAMING_MAX_ACTIVE_TASKS; i++)
    {
      if(m_scheduledNewFrame[i] > frameIndex)
      {
        info.groups += m_scheduledNew[i].groups;
        info.clusters += m_scheduledNew[i].clusters;
      }
    }
    return info;
  }


  // 鍑芥暟锛歡etNewTask銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  TaskInfo& getNewTask(uint32_t taskIndex);
  TaskInfo& getNewTask(uint32_t taskIndex);


  // 鍑芥暟锛歝mdUploadTask銆傚悜鍛戒护缂撳啿褰曞埗 GPU 鎿嶄綔锛屽苟渚濊禆澶栧眰璋冪敤鑰呭畨鎺掓彁浜や笌鍚屾銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳绫诲嚱鏁板彧鎻忚堪鍛戒护搴忓垪锛屼笉搴斿亣璁惧懡浠ゅ凡缁忕珛鍗虫墽琛屻€?  size_t cmdUploadTask(VkCommandBuffer cmd, uint32_t taskIndex);
  size_t cmdUploadTask(VkCommandBuffer cmd, uint32_t taskIndex);


  // 鍑芥暟锛歛pplyTask銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  void applyTask(shaderio::StreamingUpdate& shaderData, uint32_t taskIndex, uint32_t frameIndex);
  void applyTask(shaderio::StreamingUpdate& shaderData, uint32_t taskIndex, uint32_t frameIndex);


  const TaskInfo& getCompletedTask(uint32_t taskIndex) const { return m_taskInfos[taskIndex]; }

private:
  nvvk::BufferTyped<shaderio::StreamingPatch> m_patchesBuffer;
  nvvk::BufferTyped<shaderio::StreamingPatch> m_patchesHostBuffer;

  std::vector<nvvk::BufferSubAllocation> m_unloadHandles;
  TaskInfo                               m_taskInfos[STREAMING_MAX_ACTIVE_TASKS];

  shaderio::StreamingUpdate m_shaderData;

  uint32_t m_clusterCountAlignment;
  uint32_t m_scheduleIndex;
  NewInfo  m_pendingNew;
  NewInfo  m_scheduledNew[STREAMING_MAX_ACTIVE_TASKS]      = {};
  uint64_t m_scheduledNewFrame[STREAMING_MAX_ACTIVE_TASKS] = {};
};


// 绫诲瀷锛歋treamingStorage銆傚皝瑁呮湰妯″潡鐨勯暱鏈熺姸鎬併€佽祫婧愭墍鏈夋潈鍜屽澶栨搷浣滄帴鍙ｃ€?// 璁捐鎰忓浘锛氶€氳繃鎴愬憳鍑芥暟闆嗕腑缁存姢鐘舵€佽浆绉伙紝閬垮厤璋冪敤鏂圭洿鎺ユ嫾鎺ュ簳灞傝祫婧愮敓鍛藉懆鏈熴€?// 浣跨敤绾︽潫锛氬疄渚嬪垵濮嬪寲銆佹瘡甯т娇鐢ㄥ拰閲婃斁搴旈伒瀹堝０鏄庨『搴忓搴旂殑渚濊禆鍏崇郴銆?class StreamingStorage
class StreamingStorage
{
public:


  // 缁撴瀯锛歍askInfo銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?  // 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?  // 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?  struct TaskInfo
  struct TaskInfo
  {
    size_t usedMemory;
    size_t baseOffset;
    size_t regionCount;
  };


  // 鍑芥暟锛歩nit銆傚垵濮嬪寲鏈ā鍧楁墍闇€鐘舵€併€佽祫婧愭垨 GPU 渚х粦瀹氥€?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氬垵濮嬪寲杩囩▼寤虹珛鍚庣画闃舵鍋囧畾瀛樺湪鐨勪笉鍙橀噺锛屼緥濡傚彞鏌勬湁鏁堛€佺紦鍐插ぇ灏忚冻澶熴€佹弿杩扮宸茬粦瀹氥€?  void init(Resources& res, const StreamingConfig& config);
  void init(Resources& res, const StreamingConfig& config);


  // 鍑芥暟锛歞einit銆傞噴鏀炬垨鍥炴敹鍓嶉潰鍒濆鍖栫殑璧勬簮锛屼繚鎸佺敓鍛藉懆鏈熸垚瀵圭鐞嗐€?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氶噴鏀鹃『搴忚閬靛畧璧勬簮渚濊禆鍏崇郴锛岄伩鍏?GPU 浠嶅彲鑳借闂殑瀵硅薄琚彁鍓嶉攢姣併€?  void deinit(Resources& res);
  void deinit(Resources& res);


  // 鍑芥暟锛歳eset銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  void reset();
  void reset();


  // 鍑芥暟锛歠ree銆傞噴鏀炬垨鍥炴敹鍓嶉潰鍒濆鍖栫殑璧勬簮锛屼繚鎸佺敓鍛藉懆鏈熸垚瀵圭鐞嗐€?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氶噴鏀鹃『搴忚閬靛畧璧勬簮渚濊禆鍏崇郴锛岄伩鍏?GPU 浠嶅彲鑳借闂殑瀵硅薄琚彁鍓嶉攢姣併€?  void free(nvvk::BufferSubAllocation& handle);
  void free(nvvk::BufferSubAllocation& handle);


  // 鍑芥暟锛歡etStats銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  void   getStats(StreamingStats& stats) const;
  void   getStats(StreamingStats& stats) const;


  // 鍑芥暟锛歡etOperationsSize銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  size_t getOperationsSize() const;
  size_t getOperationsSize() const;


  // 鍑芥暟锛歡etMaxDataSize銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  size_t getMaxDataSize() const;
  size_t getMaxDataSize() const;


  // 鍑芥暟锛歡etNewTask銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  TaskInfo& getNewTask(uint32_t taskIndex);
  TaskInfo& getNewTask(uint32_t taskIndex);


  // 鍑芥暟锛歝anTransfer銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  bool canTransfer(const TaskInfo& operation, size_t size) const;
  bool canTransfer(const TaskInfo& operation, size_t size) const;


  // 鍑芥暟锛歛llocate銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  bool allocate(nvvk::BufferSubAllocation& handle, GeometryGroup group, size_t sz, uint64_t& deviceAddress);
  bool allocate(nvvk::BufferSubAllocation& handle, GeometryGroup group, size_t sz, uint64_t& deviceAddress);


  // 鍑芥暟锛歛ppendTransfer銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  void* appendTransfer(TaskInfo& operation, const nvvk::BufferSubAllocation& dstHandle);
  void* appendTransfer(TaskInfo& operation, const nvvk::BufferSubAllocation& dstHandle);


  // 鍑芥暟锛歝mdUploadTask銆傚悜鍛戒护缂撳啿褰曞埗 GPU 鎿嶄綔锛屽苟渚濊禆澶栧眰璋冪敤鑰呭畨鎺掓彁浜や笌鍚屾銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳绫诲嚱鏁板彧鎻忚堪鍛戒护搴忓垪锛屼笉搴斿亣璁惧懡浠ゅ凡缁忕珛鍗虫墽琛屻€?  uint32_t cmdUploadTask(VkCommandBuffer cmd);
  uint32_t cmdUploadTask(VkCommandBuffer cmd);

  nvvk::ManagedCommandPools m_taskCommandPool;

private:


  // 缁撴瀯锛欳opyInfo銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?  // 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?  // 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?  struct CopyInfo
  struct CopyInfo
  {
    VkBuffer targetBuffer;
    size_t   regionOffset;
    size_t   regionCount;
  };

  size_t m_maxSceneBytes;
  size_t m_maxTransferBytes;
  size_t m_blockBytes;

  nvvk::Buffer                       m_transferHostBuffer;
  nvvk::BufferSubAllocator::InitInfo m_dataInfo;
  nvvk::BufferSubAllocator           m_dataAllocator;
  std::vector<uint32_t>              m_dataQueueFamilies;

  std::vector<CopyInfo>     m_copyInfos;
  std::vector<VkBufferCopy> m_copyRegions;

  TaskInfo m_taskOperations[STREAMING_MAX_ACTIVE_TASKS];
};


// 绫诲瀷锛歋treamingTaskQueue銆傚皝瑁呮湰妯″潡鐨勯暱鏈熺姸鎬併€佽祫婧愭墍鏈夋潈鍜屽澶栨搷浣滄帴鍙ｃ€?// 璁捐鎰忓浘锛氶€氳繃鎴愬憳鍑芥暟闆嗕腑缁存姢鐘舵€佽浆绉伙紝閬垮厤璋冪敤鏂圭洿鎺ユ嫾鎺ュ簳灞傝祫婧愮敓鍛藉懆鏈熴€?// 浣跨敤绾︽潫锛氬疄渚嬪垵濮嬪寲銆佹瘡甯т娇鐢ㄥ拰閲婃斁搴旈伒瀹堝０鏄庨『搴忓搴旂殑渚濊禆鍏崇郴銆?class StreamingTaskQueue
class StreamingTaskQueue
{
public:

  static_assert(STREAMING_MAX_ACTIVE_TASKS < 32);

  StreamingTaskQueue() { m_availableTaskBits = (1 << STREAMING_MAX_ACTIVE_TASKS) - 1; }


  // 鍑芥暟锛歛cquireTaskIndex銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  uint32_t acquireTaskIndex()
  uint32_t acquireTaskIndex()
  {

    for(uint32_t i = 0; i < STREAMING_MAX_ACTIVE_TASKS; i++)
    {
      if(m_availableTaskBits & (1 << i))
      {

        m_availableTaskBits &= ~(1 << i);
        return i;
      }
    }

    return INVALID_TASK_INDEX;
  }


  // 鍑芥暟锛歳eleaseTaskIndex銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  void releaseTaskIndex(uint32_t index)
  void releaseTaskIndex(uint32_t index)
  {
    assert((m_availableTaskBits & (1 << index)) == 0);
    m_availableTaskBits |= (1 << index);
  }


  // 鍑芥暟锛歝anPop銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  bool canPop(VkDevice device, bool ensureAcquisition)
  bool canPop(VkDevice device, bool ensureAcquisition)
  {
    if(ensureAcquisition && !m_availableTaskBits && !m_taskQueue.empty())
    {


      if(m_taskQueue.front().semaphoreState.wait(device, ~0ULL) == VK_TIMEOUT)
      {

        LOGE("Failure to wait for semaphore");
        {

          exit(-1);
        }
      }
    }

    return !m_taskQueue.empty() && m_taskQueue.front().semaphoreState.testSignaled(device);
  }


  void push(uint32_t taskIndex, nvvk::SemaphoreState semaphoreState, uint32_t dependentIndex = INVALID_TASK_INDEX)
  {
    Task task = {
        .semaphoreState = semaphoreState,
        .taskIndex      = taskIndex,
        .dependentIndex = dependentIndex,
    };

    m_taskQueue.push(task);
  }


  // 鍑芥暟锛歱op銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  uint32_t pop()
  uint32_t pop()
  {
    uint32_t taskIndex = m_taskQueue.front().taskIndex;

    assert(taskIndex != INVALID_TASK_INDEX);

    m_taskQueue.pop();
    return taskIndex;
  }


  // 鍑芥暟锛歱opWithDependent銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?  uint32_t popWithDependent(uint32_t& dependentIndex)
  uint32_t popWithDependent(uint32_t& dependentIndex)
  {
    uint32_t taskIndex = m_taskQueue.front().taskIndex;

    assert(taskIndex != INVALID_TASK_INDEX);
    dependentIndex = m_taskQueue.front().dependentIndex;

    m_taskQueue.pop();
    return taskIndex;
  }

private:


  // 缁撴瀯锛歍ask銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?  // 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?  // 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?  struct Task
  struct Task
  {
    nvvk::SemaphoreState semaphoreState;
    uint32_t             taskIndex      = INVALID_TASK_INDEX;
    uint32_t             dependentIndex = INVALID_TASK_INDEX;
  };

  std::queue<Task> m_taskQueue;
  uint32_t         m_availableTaskBits;
};
}
