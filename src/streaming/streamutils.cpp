//==============================================================================
// 鏂囦欢锛歴rc/streaming/streamutils.cpp
// 妯″潡瀹氫綅锛氭祦寮忓姞杞藉熀纭€缁撴瀯瀹炵幇锛岀鐞?GPU 缂撳啿銆侀┗鐣?缁勩€佸嚑浣曞瓨鍌ㄥ垎閰嶃€佹殏瀛樹笂浼犲拰缁熻姹囨€汇€?// 鏁版嵁娴侊細杈撳叆鏄?SceneStreaming 鐨勪换鍔″拰閰嶇疆锛涜緭鍑烘槸鍙彁浜ょ殑涓婁紶鍛戒护銆佸湴鍧€淇ˉ浠诲姟鍜岀粺璁′俊鎭€?// 鏂规硶璇存槑锛氬疄鐜板眰鎶?娴佸紡鍔犺浇 鐨勭鏁ｄ簨浠惰绾︿负涓€缁勫浐瀹?缂撳啿 涓殑鎵瑰鐞嗕换鍔★紝浣?GPU 鍜?CPU 鍙互閫氳繃鏈夐檺鍚屾鐐瑰崗浣溿€?// 姝ｇ‘鎬х害鏉燂細姣忔 load/unload 閮藉繀椤诲悓姝ユ洿鏂?resident銆乻torage 鍜?鐫€鑹插櫒 鏁版嵁锛涗笂浼犺寖鍥翠笉鑳借秴杩?transfer budget銆?// 娉ㄩ噴椋庢牸锛氫娇鐢ㄤ腑鏂囪В閲?CPU 渚ц涔夛紱淇濈暀蹇呰鐨?API銆佺被鍨嬪悕鍜屾暟瀛︾缉鍐欎互渚挎绱€?//==============================================================================
// 渚濊禆璇存槑锛氬紩鍏ユ湰缂栬瘧鍗曞厓闇€瑕佺殑澶栭儴搴撱€侀」鐩ā鍧楀拰鍏变韩鐫€鑹插櫒甯冨眬銆?// 渚濊禆椤哄簭閫氬父鍙嶆槧鎶借薄灞傛锛氬厛澶栭儴搴擄紝鍐嶉」鐩ā鍧楋紝鏈€鍚庝笌 GPU 鍏变韩鐨勬帴鍙ｅ畾涔夈€?#include <volk.h>
#include <volk.h>
#include "streamutils.hpp"


// 鍛藉悕绌洪棿璇存槑锛氶檺鍒剁鍙峰彲瑙佽寖鍥达紝骞惰〃鏄庤繖浜涚被鍨嬪拰鍑芥暟灞炰簬鍚屼竴鍔熻兘鍩熴€?// 璇ヨ竟鐣屾湁鍔╀簬鍖哄垎搴旂敤灞傘€佹覆鏌撳眰銆佸満鏅眰鍜岀畻娉曞眰鐨勮亴璐ｃ€?namespace lodclusters {
namespace lodclusters {


// 鍑芥暟锛歋treamingRequests::init銆傚垵濮嬪寲鏈ā鍧楁墍闇€鐘舵€併€佽祫婧愭垨 GPU 渚х粦瀹氥€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氬垵濮嬪寲杩囩▼寤虹珛鍚庣画闃舵鍋囧畾瀛樺湪鐨勪笉鍙橀噺锛屼緥濡傚彞鏌勬湁鏁堛€佺紦鍐插ぇ灏忚冻澶熴€佹弿杩扮宸茬粦瀹氥€?void StreamingRequests::init(Resources& res, const StreamingConfig& config, uint32_t groupCountAlignment, uint32_t clusterCountAlignment)
void StreamingRequests::init(Resources& res, const StreamingConfig& config, uint32_t groupCountAlignment, uint32_t clusterCountAlignment)
{
  m_shaderData            = {};
  m_shaderData.maxLoads   = config.maxPerFrameLoadRequests;
  m_shaderData.maxUnloads = config.maxPerFrameUnloadRequests;


  BufferRanges ranges = {};
  m_shaderData.loadGeometryGroups =
      ranges.append(sizeof(GeometryGroup) * nvutils::align_up(config.maxPerFrameLoadRequests, groupCountAlignment), 8);
  m_shaderData.unloadGeometryGroups =
      ranges.append(sizeof(GeometryGroup) * nvutils::align_up(config.maxPerFrameUnloadRequests, groupCountAlignment), 8);

  m_requestSize = ranges.getSize();


  m_shaderDataOffset = m_requestSize * STREAMING_MAX_ACTIVE_TASKS;

  std::vector<uint32_t> sharingQueueFamilies;
  if(config.useAsyncTransfer)
  {

    sharingQueueFamilies.push_back(res.m_queueStates.primary.m_familyIndex);

    sharingQueueFamilies.push_back(res.m_queueStates.transfer.m_familyIndex);
  }

  res.createBuffer(m_requestBuffer, (m_requestSize + sizeof(shaderio::StreamingRequest)) * STREAMING_MAX_ACTIVE_TASKS,
                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 0, 0, sharingQueueFamilies);

  NVVK_DBG_NAME(m_requestBuffer.buffer);

  res.createBuffer(m_requestHostBuffer, m_requestBuffer.bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
                   VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);

  NVVK_DBG_NAME(m_requestHostBuffer.buffer);

  for(uint32_t c = 0; c < STREAMING_MAX_ACTIVE_TASKS; c++)
  {
    TaskInfo& task = m_taskInfos[c];
    task           = {};

    task.shaderData = reinterpret_cast<const shaderio::StreamingRequest*>(
        uint64_t(m_requestHostBuffer.mapping) + m_shaderDataOffset + sizeof(shaderio::StreamingRequest) * c);
    task.loadGeometryGroups = reinterpret_cast<const GeometryGroup*>(
        uint64_t(m_requestHostBuffer.mapping) + m_requestSize * c + m_shaderData.loadGeometryGroups);
    task.unloadGeometryGroups = reinterpret_cast<const GeometryGroup*>(
        uint64_t(m_requestHostBuffer.mapping) + m_requestSize * c + m_shaderData.unloadGeometryGroups);
  }
}


// 鍑芥暟锛歋treamingRequests::deinit銆傞噴鏀炬垨鍥炴敹鍓嶉潰鍒濆鍖栫殑璧勬簮锛屼繚鎸佺敓鍛藉懆鏈熸垚瀵圭鐞嗐€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氶噴鏀鹃『搴忚閬靛畧璧勬簮渚濊禆鍏崇郴锛岄伩鍏?GPU 浠嶅彲鑳借闂殑瀵硅薄琚彁鍓嶉攢姣併€?void StreamingRequests::deinit(Resources& res)
void StreamingRequests::deinit(Resources& res)
{

  res.m_allocator.destroyBuffer(m_requestBuffer);

  res.m_allocator.destroyBuffer(m_requestHostBuffer);
}


// 鍑芥暟锛歋treamingRequests::getOperationsSize銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?size_t StreamingRequests::getOperationsSize() const
size_t StreamingRequests::getOperationsSize() const
{
  return m_requestBuffer.bufferSize;
}


// 鍑芥暟锛歋treamingRequests::applyTask銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?void StreamingRequests::applyTask(shaderio::StreamingRequest& shaderData, uint32_t taskIndex, uint32_t frameIndex)
void StreamingRequests::applyTask(shaderio::StreamingRequest& shaderData, uint32_t taskIndex, uint32_t frameIndex)
{
  shaderData = m_shaderData;
  shaderData.loadGeometryGroups += m_requestBuffer.address + m_requestSize * taskIndex;
  shaderData.unloadGeometryGroups += m_requestBuffer.address + m_requestSize * taskIndex;
  shaderData.taskIndex = taskIndex;


  shaderData.frameIndex = STREAMING_INVALID_ADDRESS_START + frameIndex;
}


// 鍑芥暟锛歋treamingRequests::cmdRunTask銆傚悜鍛戒护缂撳啿褰曞埗 GPU 鎿嶄綔锛屽苟渚濊禆澶栧眰璋冪敤鑰呭畨鎺掓彁浜や笌鍚屾銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳绫诲嚱鏁板彧鎻忚堪鍛戒护搴忓垪锛屼笉搴斿亣璁惧懡浠ゅ凡缁忕珛鍗虫墽琛屻€?void StreamingRequests::cmdRunTask(VkCommandBuffer cmd, const shaderio::StreamingRequest& shaderData, VkBuffer buffer, size_t bufferOffset)
void StreamingRequests::cmdRunTask(VkCommandBuffer cmd, const shaderio::StreamingRequest& shaderData, VkBuffer buffer, size_t bufferOffset)
{
  uint32_t taskIndex = shaderData.taskIndex;


  VkBufferCopy region;
  region.dstOffset = m_requestSize * taskIndex;
  region.srcOffset = m_requestSize * taskIndex;
  region.size      = m_requestSize;

  vkCmdCopyBuffer(cmd, m_requestBuffer.buffer, m_requestHostBuffer.buffer, 1, &region);


  region.dstOffset = m_shaderDataOffset + (sizeof(shaderio::StreamingRequest) * taskIndex);
  region.srcOffset = bufferOffset;
  region.size      = sizeof(shaderio::StreamingRequest);

  vkCmdCopyBuffer(cmd, buffer, m_requestHostBuffer.buffer, 1, &region);
}


// 鍑芥暟锛歋treamingResident::init銆傚垵濮嬪寲鏈ā鍧楁墍闇€鐘舵€併€佽祫婧愭垨 GPU 渚х粦瀹氥€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氬垵濮嬪寲杩囩▼寤虹珛鍚庣画闃舵鍋囧畾瀛樺湪鐨勪笉鍙橀噺锛屼緥濡傚彞鏌勬湁鏁堛€佺紦鍐插ぇ灏忚冻澶熴€佹弿杩扮宸茬粦瀹氥€?void StreamingResident::init(Resources& res, const StreamingConfig& config, uint32_t groupCountAlignment, uint32_t clusterCountAlignment)
void StreamingResident::init(Resources& res, const StreamingConfig& config, uint32_t groupCountAlignment, uint32_t clusterCountAlignment)
{

  m_groupAllocator.init(config.maxGroups);

  m_clusterAllocator.init(config.maxClusters);


  m_maxClusters  = nvutils::align_up(config.maxClusters, clusterCountAlignment);

  m_maxGroups    = nvutils::align_up(config.maxGroups, groupCountAlignment);
  m_lowDetailGroupsCount   = 0;
  m_lowDetailClustersCount = 0;

  m_activeGroupsCount   = 0;
  m_activeClustersCount = 0;

  m_groupIndicesUpdateRange    = {};
  m_groups                     = {};
  m_activeGroupIndices         = {};
  m_mapGeometryGroup2Residency = {};


  m_mapGeometryGroup2Residency.reserve(m_maxGroups);

  m_groups.resize(m_maxGroups);

  m_activeGroupIndices.resize(m_maxGroups);

  BufferRanges ranges      = {};
  m_residentGroupsOffset   = ranges.append(sizeof(shaderio::StreamingGroup) * m_maxGroups, 16);
  m_residentGroupIDsOffset = ranges.append(sizeof(shaderio::uint32_t) * m_maxGroups, 4);
  m_residentClustersOffset = ranges.append(sizeof(shaderio::uint64_t) * m_maxClusters, 8);
  m_residentActiveOffset   = ranges.append(sizeof(shaderio::uint32_t) * m_maxGroups, 4);
  m_residentActiveUpdateOffset = ranges.append(sizeof(shaderio::uint32_t) * m_maxGroups * STREAMING_MAX_ACTIVE_TASKS, 4);

  std::vector<uint32_t> sharingQueueFamilies;
  if(config.useAsyncTransfer)
  {

    sharingQueueFamilies.push_back(res.m_queueStates.primary.m_familyIndex);

    sharingQueueFamilies.push_back(res.m_queueStates.transfer.m_familyIndex);
  }

  res.createBuffer(m_residentBuffer, ranges.getSize(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 0, 0, sharingQueueFamilies);

  NVVK_DBG_NAME(m_residentBuffer.buffer);

  res.createBufferTyped(m_residentActiveHostBuffer, (m_maxGroups)*STREAMING_MAX_ACTIVE_TASKS,
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY,
                        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);

  NVVK_DBG_NAME(m_residentActiveHostBuffer.buffer);

  m_shaderData              = {};
  m_shaderData.groups       = m_residentBuffer.address + m_residentGroupsOffset;
  m_shaderData.groupIDs     = m_residentBuffer.address + m_residentGroupIDsOffset;
  m_shaderData.clusters     = m_residentBuffer.address + m_residentClustersOffset;
  m_shaderData.activeGroups = m_residentBuffer.address + m_residentActiveOffset;
}


// 鍑芥暟锛歋treamingResident::deinit銆傞噴鏀炬垨鍥炴敹鍓嶉潰鍒濆鍖栫殑璧勬簮锛屼繚鎸佺敓鍛藉懆鏈熸垚瀵圭鐞嗐€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氶噴鏀鹃『搴忚閬靛畧璧勬簮渚濊禆鍏崇郴锛岄伩鍏?GPU 浠嶅彲鑳借闂殑瀵硅薄琚彁鍓嶉攢姣併€?void StreamingResident::deinit(Resources& res)
void StreamingResident::deinit(Resources& res)
{

  m_groupAllocator.destroyAll();

  m_clusterAllocator.destroyAll();


  res.m_allocator.destroyBuffer(m_residentBuffer);

  res.m_allocator.destroyBuffer(m_residentActiveHostBuffer);

  *this = {};
}


// 鍑芥暟锛歋treamingResident::getOperationsSize銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?size_t StreamingResident::getOperationsSize() const
size_t StreamingResident::getOperationsSize() const
{
  return m_residentBuffer.bufferSize;
}

// 鍑芥暟锛歋treamingResident::getStats銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?void StreamingResident::getStats(StreamingStats& stats) const
void StreamingResident::getStats(StreamingStats& stats) const
{
  stats.residentGroups     = m_activeGroupsCount;
  stats.residentClusters   = m_activeClustersCount;
  stats.persistentGroups   = m_lowDetailGroupsCount;
  stats.persistentClusters = m_lowDetailClustersCount;
}


// 鍑芥暟锛歋treamingResident::reset銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?void StreamingResident::reset(shaderio::StreamingResident& shaderData)
void StreamingResident::reset(shaderio::StreamingResident& shaderData)
{
  for(uint32_t activeGroup = m_lowDetailGroupsCount; activeGroup < m_activeGroupsCount; activeGroup++)
  {
    Group& group = m_groups[m_activeGroupIndices[activeGroup]];


    m_mapGeometryGroup2Residency.erase(group.geometryGroup.key);

    m_groupAllocator.destroyID(group.groupResidentID);

    m_clusterAllocator.destroyRangeID(group.clusterResidentID, group.clusterCount);
  }

  m_activeGroupsCount   = m_lowDetailGroupsCount;
  m_activeClustersCount = m_lowDetailClustersCount;

  m_groupIndicesUpdateRange = {};


  m_shaderData.activeGroupsCount   = m_activeGroupsCount - m_lowDetailGroupsCount;
  m_shaderData.activeClustersCount = m_activeClustersCount - m_lowDetailClustersCount;
  m_shaderData.activeGroups = m_residentBuffer.address + m_residentActiveOffset + sizeof(uint32_t) * m_lowDetailGroupsCount;

  shaderData = m_shaderData;
}


// 鍑芥暟锛歋treamingResident::uploadInitialState銆傚垵濮嬪寲鏈ā鍧楁墍闇€鐘舵€併€佽祫婧愭垨 GPU 渚х粦瀹氥€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氬垵濮嬪寲杩囩▼寤虹珛鍚庣画闃舵鍋囧畾瀛樺湪鐨勪笉鍙橀噺锛屼緥濡傚彞鏌勬湁鏁堛€佺紦鍐插ぇ灏忚冻澶熴€佹弿杩扮宸茬粦瀹氥€?void StreamingResident::uploadInitialState(Resources::BatchedUploader& uploader, shaderio::StreamingResident& shaderData)
void StreamingResident::uploadInitialState(Resources::BatchedUploader& uploader, shaderio::StreamingResident& shaderData)
{


  m_lowDetailGroupsCount      = m_activeGroupsCount;
  m_lowDetailClustersCount    = m_activeClustersCount;
  m_lowDetailMaxGroupClusters = 0;


  uint32_t updatedActiveGroups = m_lowDetailGroupsCount;
#if STREAMING_DEBUG_ADDRESSES
  updatedActiveGroups = m_maxGroups;
#endif

  shaderio::StreamingGroup* shaderGroups =
      uploader.uploadBuffer(m_residentBuffer, m_residentGroupsOffset,
                            sizeof(shaderio::StreamingGroup) * updatedActiveGroups, (shaderio::StreamingGroup*)nullptr);

  uint64_t* shaderClusters = uploader.uploadBuffer(m_residentBuffer, m_residentClustersOffset,
                                                   sizeof(shaderio::uint64_t) * m_activeClustersCount,
                                                   (uint64_t*)nullptr, Resources::DONT_FLUSH);

  for(uint32_t g = 0; g < m_lowDetailGroupsCount; g++)
  {
    const Group& group = m_groups[g];

    assert(group.groupResidentID == g);

    shaderio::StreamingGroup& shaderGroup = shaderGroups[g];
    shaderGroup.age                       = 0x1234;
    shaderGroup.lodLevel                  = group.lodLevel;
    shaderGroup.group                     = group.deviceAddress;
    for(uint32_t c = 0; c < group.clusterCount; c++)
    {
      shaderClusters[group.clusterResidentID + c] = group.deviceAddress + sizeof(shaderio::Group) + sizeof(shaderio::Cluster) * c;
    }
    m_lowDetailMaxGroupClusters = std::max(m_lowDetailMaxGroupClusters, uint32_t(group.clusterCount));
  }
#if STREAMING_DEBUG_ADDRESSES

  for(uint32_t g = m_lowDetailGroupsCount; g < updatedActiveGroups; g++)
  {
    shaderGroups[g].group = STREAMING_INVALID_ADDRESS_START;
  }
#endif


  m_shaderData.activeGroupsCount   = m_activeGroupsCount - m_lowDetailGroupsCount;
  m_shaderData.activeClustersCount = m_activeClustersCount - m_lowDetailClustersCount;
  m_shaderData.activeGroups = m_residentBuffer.address + m_residentActiveOffset + sizeof(uint32_t) * m_lowDetailGroupsCount;

  shaderData = m_shaderData;
}


// 鍑芥暟锛歋treamingResident::cmdUploadTask銆傚悜鍛戒护缂撳啿褰曞埗 GPU 鎿嶄綔锛屽苟渚濊禆澶栧眰璋冪敤鑰呭畨鎺掓彁浜や笌鍚屾銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳绫诲嚱鏁板彧鎻忚堪鍛戒护搴忓垪锛屼笉搴斿亣璁惧懡浠ゅ凡缁忕珛鍗虫墽琛屻€?size_t StreamingResident::cmdUploadTask(VkCommandBuffer cmd, uint32_t taskIndex)
size_t StreamingResident::cmdUploadTask(VkCommandBuffer cmd, uint32_t taskIndex)
{
  TaskInfo& task = m_taskInfos[taskIndex];

  uint32_t taskOffset = m_maxGroups * taskIndex;

  task.region     = {};
  task.shaderData = m_shaderData;

  task.shaderData.activeGroupsCount   = m_activeGroupsCount - m_lowDetailGroupsCount;
  task.shaderData.activeClustersCount = m_activeClustersCount - m_lowDetailClustersCount;


  uint32_t deltaCount = m_groupIndicesUpdateRange.count();
  if(!deltaCount)
  {
    return 0;
  }


  memcpy(m_residentActiveHostBuffer.data() + taskOffset, m_activeGroupIndices.data() + m_groupIndicesUpdateRange.lo,
         sizeof(uint32_t) * deltaCount);


  VkBufferCopy region;
  region.size      = sizeof(uint32_t) * deltaCount;
  region.srcOffset = sizeof(uint32_t) * taskOffset;
  region.dstOffset = m_residentActiveUpdateOffset + sizeof(uint32_t) * (taskOffset);

  vkCmdCopyBuffer(cmd, m_residentActiveHostBuffer.buffer, m_residentBuffer.buffer, 1, &region);


  task.region.size      = region.size;
  task.region.srcOffset = region.dstOffset;

  task.region.dstOffset = m_residentActiveOffset + sizeof(uint32_t) * m_groupIndicesUpdateRange.lo;


  m_groupIndicesUpdateRange = {};

  return region.size;
}


// 鍑芥暟锛歋treamingResident::applyTask銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?void StreamingResident::applyTask(shaderio::StreamingResident& shaderData, uint32_t taskIndex, uint32_t frameIndex)
void StreamingResident::applyTask(shaderio::StreamingResident& shaderData, uint32_t taskIndex, uint32_t frameIndex)
{
  shaderData            = m_taskInfos[taskIndex].shaderData;
  shaderData.taskIndex  = taskIndex;
  shaderData.frameIndex = frameIndex;
}


// 鍑芥暟锛歋treamingResident::cmdRunTask銆傚悜鍛戒护缂撳啿褰曞埗 GPU 鎿嶄綔锛屽苟渚濊禆澶栧眰璋冪敤鑰呭畨鎺掓彁浜や笌鍚屾銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳绫诲嚱鏁板彧鎻忚堪鍛戒护搴忓垪锛屼笉搴斿亣璁惧懡浠ゅ凡缁忕珛鍗虫墽琛屻€?void StreamingResident::cmdRunTask(VkCommandBuffer cmd, uint32_t taskIndex)
void StreamingResident::cmdRunTask(VkCommandBuffer cmd, uint32_t taskIndex)
{
  TaskInfo& task = m_taskInfos[taskIndex];
  if(task.region.size)
  {

    vkCmdCopyBuffer(cmd, m_residentBuffer.buffer, m_residentBuffer.buffer, 1, &task.region);
  }
}


// 鍑芥暟锛歋treamingResident::getLoadActiveGroupsOffset銆備粠鏂囦欢銆佺紦瀛樸€丟PU 缂撳啿鎴栧叡浜竷灞€涓鍙栨暟鎹苟杞崲涓烘湰妯″潡鏍煎紡銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍙栬矾寰勯渶瑕佹牎楠岃緭鍏ュ悎娉曟€э紝骞舵妸澶栭儴鏍煎紡鐨勪笉纭畾鎬ц浆鍖栦负鍐呴儴纭畾甯冨眬銆?uint32_t StreamingResident::getLoadActiveGroupsOffset() const
uint32_t StreamingResident::getLoadActiveGroupsOffset() const
{
  return m_activeGroupsCount - m_lowDetailGroupsCount;
}


// 鍑芥暟锛歋treamingResident::getLoadActiveClustersOffset銆備粠鏂囦欢銆佺紦瀛樸€丟PU 缂撳啿鎴栧叡浜竷灞€涓鍙栨暟鎹苟杞崲涓烘湰妯″潡鏍煎紡銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍙栬矾寰勯渶瑕佹牎楠岃緭鍏ュ悎娉曟€э紝骞舵妸澶栭儴鏍煎紡鐨勪笉纭畾鎬ц浆鍖栦负鍐呴儴纭畾甯冨眬銆?uint32_t StreamingResident::getLoadActiveClustersOffset() const
uint32_t StreamingResident::getLoadActiveClustersOffset() const
{
  return m_activeClustersCount - m_lowDetailClustersCount;
}


// 鍑芥暟锛歋treamingResident::canAllocateGroup銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?bool StreamingResident::canAllocateGroup(uint32_t numClusters) const
bool StreamingResident::canAllocateGroup(uint32_t numClusters) const
{
  return m_groupAllocator.isRangeAvailable(1) && m_clusterAllocator.isRangeAvailable(numClusters);
}


// 鍑芥暟锛歋treamingResident::findGroup銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?const StreamingResident::Group* StreamingResident::findGroup(GeometryGroup geometryGroup) const
const StreamingResident::Group* StreamingResident::findGroup(GeometryGroup geometryGroup) const
{

  auto it = m_mapGeometryGroup2Residency.find(geometryGroup.key);
  if(it == m_mapGeometryGroup2Residency.end())
  {
    return nullptr;
  }
  else
  {
    return &m_groups[it->second];
  }
}


// 鍑芥暟锛歋treamingResident::addGroup銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?StreamingResident::Group* StreamingResident::addGroup(GeometryGroup geometryGroup, uint32_t clusterCount)
StreamingResident::Group* StreamingResident::addGroup(GeometryGroup geometryGroup, uint32_t clusterCount)
{
  bool     valid = false;
  uint32_t groupResidentID;
  uint32_t clusterResidentID;

  valid = m_groupAllocator.createID(groupResidentID);

  assert(valid);

  valid = m_clusterAllocator.createRangeID(clusterResidentID, clusterCount);

  assert(valid);

  StreamingResident::Group& group = m_groups[groupResidentID];

  assert(m_mapGeometryGroup2Residency.find(geometryGroup.key) == m_mapGeometryGroup2Residency.end());
  m_mapGeometryGroup2Residency.insert({geometryGroup.key, groupResidentID});

  group.activeIndex       = m_activeGroupsCount++;
  group.geometryGroup     = geometryGroup;
  group.groupResidentID   = groupResidentID;
  group.clusterResidentID = clusterResidentID;
  group.clusterCount      = clusterCount;
  group.deviceAddress     = STREAMING_INVALID_ADDRESS_START;

  m_activeGroupIndices[group.activeIndex] = groupResidentID;


  m_activeClustersCount += clusterCount;

  return &m_groups[groupResidentID];
}


// 鍑芥暟锛歋treamingResident::removeGroup銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?void StreamingResident::removeGroup(uint32_t groupResidentID)
void StreamingResident::removeGroup(uint32_t groupResidentID)
{
  StreamingResident::Group& group = m_groups[groupResidentID];
  assert(m_mapGeometryGroup2Residency.find(group.geometryGroup.key) != m_mapGeometryGroup2Residency.end());

  m_mapGeometryGroup2Residency.erase(group.geometryGroup.key);

  {

    uint32_t activeIndex = group.activeIndex;


    if(activeIndex + 1 != m_activeGroupsCount)
    {
      uint32_t lastResidentID              = m_activeGroupIndices[m_activeGroupsCount - 1];
      m_groups[lastResidentID].activeIndex = activeIndex;
      m_activeGroupIndices[activeIndex]    = lastResidentID;


      m_groupIndicesUpdateRange.update(activeIndex);
    }
    m_activeGroupsCount--;
  }

  m_activeClustersCount -= group.clusterCount;


  m_groupAllocator.destroyID(groupResidentID);

  m_clusterAllocator.destroyRangeID(group.clusterResidentID, group.clusterCount);

  group = {};
}




// 鍑芥暟锛歋treamingUpdates::init銆傚垵濮嬪寲鏈ā鍧楁墍闇€鐘舵€併€佽祫婧愭垨 GPU 渚х粦瀹氥€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氬垵濮嬪寲杩囩▼寤虹珛鍚庣画闃舵鍋囧畾瀛樺湪鐨勪笉鍙橀噺锛屼緥濡傚彞鏌勬湁鏁堛€佺紦鍐插ぇ灏忚冻澶熴€佹弿杩扮宸茬粦瀹氥€?void StreamingUpdates::init(Resources& res, const StreamingConfig& config, uint32_t groupCountAlignment, uint32_t clusterCountAlignment)
void StreamingUpdates::init(Resources& res, const StreamingConfig& config, uint32_t groupCountAlignment, uint32_t clusterCountAlignment)
{
  m_clusterCountAlignment = clusterCountAlignment;
  m_scheduleIndex         = 0;
  m_pendingNew            = {};

  memset(m_scheduledNew, 0, sizeof(m_scheduledNew));
  memset(m_scheduledNewFrame, 0, sizeof(m_scheduledNewFrame));


  uint32_t loadRequests   = nvutils::align_up(config.maxPerFrameLoadRequests, groupCountAlignment);

  uint32_t unloadRequests = nvutils::align_up(config.maxPerFrameUnloadRequests, groupCountAlignment);

  m_shaderData                        = {};
  m_shaderData.patchGroupsCount       = loadRequests + unloadRequests;
  m_shaderData.patchUnloadGroupsCount = unloadRequests;

  uint32_t framePatchCount = m_shaderData.patchGroupsCount;

  m_unloadHandles = {};

  m_unloadHandles.resize(unloadRequests * STREAMING_MAX_ACTIVE_TASKS);

  std::vector<uint32_t> sharingQueueFamilies;
  if(config.useAsyncTransfer)
  {

    sharingQueueFamilies.push_back(res.m_queueStates.primary.m_familyIndex);

    sharingQueueFamilies.push_back(res.m_queueStates.transfer.m_familyIndex);
  }

  res.createBufferTyped(m_patchesBuffer, framePatchCount * STREAMING_MAX_ACTIVE_TASKS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, 0, 0, sharingQueueFamilies);

  NVVK_DBG_NAME(m_patchesBuffer.buffer);

  res.createBufferTyped(m_patchesHostBuffer, framePatchCount * STREAMING_MAX_ACTIVE_TASKS, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);

  NVVK_DBG_NAME(m_patchesHostBuffer.buffer);

  m_shaderData.patches = m_patchesBuffer.address;

  for(uint32_t c = 0; c < STREAMING_MAX_ACTIVE_TASKS; c++)
  {
    StreamingUpdates::TaskInfo& task = m_taskInfos[c];
    task.unloadPatches               = m_patchesHostBuffer.data() + framePatchCount * c;
    task.loadPatches                 = task.unloadPatches + unloadRequests;
    task.unloadHandles = m_unloadHandles.data() + unloadRequests * c;
  }
}


// 鍑芥暟锛歋treamingUpdates::getOperationsSize銆傛牴鎹渶鏂扮姸鎬佸埛鏂扮紦瀛樻暟鎹€丟PU 鍦板潃銆佹弿杩扮鎴栫粺璁′俊鎭€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氭洿鏂板嚱鏁拌礋璐ｆ妸鈥滄棫鐘舵€佲€濇帹杩涘埌鈥滃綋鍓嶇姸鎬佲€濓紝鍥犳瑕侀伩鍏嶉儴鍒嗘洿鏂伴€犳垚 CPU/GPU 瑙嗗浘涓嶄竴鑷淬€?size_t StreamingUpdates::getOperationsSize() const
size_t StreamingUpdates::getOperationsSize() const
{
  return m_patchesBuffer.bufferSize;
}


// 鍑芥暟锛歋treamingUpdates::deinit銆傞噴鏀炬垨鍥炴敹鍓嶉潰鍒濆鍖栫殑璧勬簮锛屼繚鎸佺敓鍛藉懆鏈熸垚瀵圭鐞嗐€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氶噴鏀鹃『搴忚閬靛畧璧勬簮渚濊禆鍏崇郴锛岄伩鍏?GPU 浠嶅彲鑳借闂殑瀵硅薄琚彁鍓嶉攢姣併€?void StreamingUpdates::deinit(Resources& res)
void StreamingUpdates::deinit(Resources& res)
{

  res.m_allocator.destroyBuffer(m_patchesBuffer);

  res.m_allocator.destroyBuffer(m_patchesHostBuffer);
}


// 鍑芥暟锛歋treamingUpdates::reset銆傛牴鎹渶鏂扮姸鎬佸埛鏂扮紦瀛樻暟鎹€丟PU 鍦板潃銆佹弿杩扮鎴栫粺璁′俊鎭€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氭洿鏂板嚱鏁拌礋璐ｆ妸鈥滄棫鐘舵€佲€濇帹杩涘埌鈥滃綋鍓嶇姸鎬佲€濓紝鍥犳瑕侀伩鍏嶉儴鍒嗘洿鏂伴€犳垚 CPU/GPU 瑙嗗浘涓嶄竴鑷淬€?void StreamingUpdates::reset()
void StreamingUpdates::reset()
{
  m_pendingNew = {};
  memset(m_scheduledNew, 0, sizeof(m_scheduledNew));
  memset(m_scheduledNewFrame, 0, sizeof(m_scheduledNewFrame));
  m_scheduleIndex = 0;
}


// 鍑芥暟锛歋treamingUpdates::getNewTask銆傛牴鎹渶鏂扮姸鎬佸埛鏂扮紦瀛樻暟鎹€丟PU 鍦板潃銆佹弿杩扮鎴栫粺璁′俊鎭€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氭洿鏂板嚱鏁拌礋璐ｆ妸鈥滄棫鐘舵€佲€濇帹杩涘埌鈥滃綋鍓嶇姸鎬佲€濓紝鍥犳瑕侀伩鍏嶉儴鍒嗘洿鏂伴€犳垚 CPU/GPU 瑙嗗浘涓嶄竴鑷淬€?lodclusters::StreamingUpdates::TaskInfo& StreamingUpdates::getNewTask(uint32_t taskIndex)
lodclusters::StreamingUpdates::TaskInfo& StreamingUpdates::getNewTask(uint32_t taskIndex)
{
  TaskInfo& task                   = m_taskInfos[taskIndex];
  task.loadCount                   = 0;
  task.unloadCount                 = 0;
  task.newClusterCount             = 0;
  task.loadActiveGroupsOffset      = ~0;
  task.loadActiveClustersOffset    = ~0;

  return task;
}


// 鍑芥暟锛歋treamingUpdates::cmdUploadTask銆傚悜鍛戒护缂撳啿褰曞埗 GPU 鎿嶄綔锛屽苟渚濊禆澶栧眰璋冪敤鑰呭畨鎺掓彁浜や笌鍚屾銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳绫诲嚱鏁板彧鎻忚堪鍛戒护搴忓垪锛屼笉搴斿亣璁惧懡浠ゅ凡缁忕珛鍗虫墽琛屻€?size_t StreamingUpdates::cmdUploadTask(VkCommandBuffer cmd, uint32_t taskIndex)
size_t StreamingUpdates::cmdUploadTask(VkCommandBuffer cmd, uint32_t taskIndex)
{
  const TaskInfo& task = m_taskInfos[taskIndex];


  assert(task.loadActiveGroupsOffset != ~0);

  assert(task.loadActiveClustersOffset != ~0);

  size_t transferSize = 0;


  VkBufferCopy regions[2];
  uint32_t     regionCount = 0;

  uint32_t framePatchCount = m_shaderData.patchGroupsCount;

  regions[0].srcOffset = sizeof(shaderio::StreamingPatch) * framePatchCount * taskIndex;
  regions[1].srcOffset = sizeof(shaderio::StreamingPatch) * framePatchCount * taskIndex;
  regions[0].dstOffset = sizeof(shaderio::StreamingPatch) * framePatchCount * taskIndex;

  if(task.unloadCount)
  {

    regions[regionCount].size = sizeof(shaderio::StreamingPatch) * (task.unloadCount);

    regions[regionCount].srcOffset += 0;


    regions[regionCount + 1].dstOffset = regions[regionCount].dstOffset + regions[regionCount].size;

    transferSize += regions[0].size;

    regionCount++;
  }

  if(task.loadCount)
  {

    regions[regionCount].size = sizeof(shaderio::StreamingPatch) * (task.loadCount);

    regions[regionCount].srcOffset += sizeof(shaderio::StreamingPatch) * m_shaderData.patchUnloadGroupsCount;


    regions[regionCount + 1].dstOffset = regions[regionCount].dstOffset + regions[regionCount].size;

    transferSize += regions[regionCount].size;

    regionCount++;
  }

  if(regionCount)
  {

    vkCmdCopyBuffer(cmd, m_patchesHostBuffer.buffer, m_patchesBuffer.buffer, regionCount, regions);
  }


  m_pendingNew.clusters += task.newClusterCount;
  m_pendingNew.groups += task.loadCount;

  return transferSize;
}


// 鍑芥暟锛歋treamingUpdates::applyTask銆傛牴鎹渶鏂扮姸鎬佸埛鏂扮紦瀛樻暟鎹€丟PU 鍦板潃銆佹弿杩扮鎴栫粺璁′俊鎭€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氭洿鏂板嚱鏁拌礋璐ｆ妸鈥滄棫鐘舵€佲€濇帹杩涘埌鈥滃綋鍓嶇姸鎬佲€濓紝鍥犳瑕侀伩鍏嶉儴鍒嗘洿鏂伴€犳垚 CPU/GPU 瑙嗗浘涓嶄竴鑷淬€?void StreamingUpdates::applyTask(shaderio::StreamingUpdate& shaderData, uint32_t taskIndex, uint32_t frameIndex)
void StreamingUpdates::applyTask(shaderio::StreamingUpdate& shaderData, uint32_t taskIndex, uint32_t frameIndex)
{
  uint32_t framePatchCount = m_shaderData.patchGroupsCount;

  const TaskInfo& task = m_taskInfos[taskIndex];

  shaderData = m_shaderData;

  shaderData.patchGroupsCount         = task.loadCount + task.unloadCount;
  shaderData.patchUnloadGroupsCount   = task.unloadCount;
  shaderData.patches += sizeof(shaderio::StreamingPatch) * framePatchCount * taskIndex;
  shaderData.taskIndex       = taskIndex;
  shaderData.frameIndex      = frameIndex;
  shaderData.loadActiveGroupsOffset   = task.loadActiveGroupsOffset;
  shaderData.loadActiveClustersOffset = task.loadActiveClustersOffset;


  assert(m_pendingNew.clusters >= task.newClusterCount);

  assert(m_pendingNew.groups >= task.loadCount);

  m_pendingNew.clusters -= task.newClusterCount;
  m_pendingNew.groups -= task.loadCount;
  m_scheduledNewFrame[m_scheduleIndex % STREAMING_MAX_ACTIVE_TASKS]     = frameIndex;
  m_scheduledNew[m_scheduleIndex % STREAMING_MAX_ACTIVE_TASKS].clusters = task.newClusterCount;
  m_scheduledNew[m_scheduleIndex % STREAMING_MAX_ACTIVE_TASKS].groups   = task.loadCount;
  m_scheduleIndex++;
}


// 鍑芥暟锛歋treamingStorage::init銆傚垵濮嬪寲鏈ā鍧楁墍闇€鐘舵€併€佽祫婧愭垨 GPU 渚х粦瀹氥€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氬垵濮嬪寲杩囩▼寤虹珛鍚庣画闃舵鍋囧畾瀛樺湪鐨勪笉鍙橀噺锛屼緥濡傚彞鏌勬湁鏁堛€佺紦鍐插ぇ灏忚冻澶熴€佹弿杩扮宸茬粦瀹氥€?void StreamingStorage::init(Resources& res, const StreamingConfig& config)
void StreamingStorage::init(Resources& res, const StreamingConfig& config)
{
  m_maxSceneBytes    = config.maxGeometryMegaBytes * 1024 * 1024;
  m_maxTransferBytes = config.maxTransferMegaBytes * 1024 * 1024;


  if(m_maxSceneBytes > 4 * 1024 * 1024 * 1024)
    m_blockBytes = std::min(size_t(256) * 1024 * 1024, m_maxSceneBytes);
  else if(m_maxSceneBytes > 1 * 1024 * 1024 * 1024)
    m_blockBytes = std::min(size_t(128) * 1024 * 1024, m_maxSceneBytes);
  else
    m_blockBytes = std::min(size_t(64) * 1024 * 1024, m_maxSceneBytes);

  m_dataQueueFamilies = {};
  if(config.useAsyncTransfer)
  {

    m_dataQueueFamilies.push_back(res.m_queueStates.primary.m_familyIndex);

    m_dataQueueFamilies.push_back(res.m_queueStates.transfer.m_familyIndex);

    m_taskCommandPool.init(res.m_device, res.m_queueStates.transfer.m_familyIndex, nvvk::ManagedCommandPools::Mode::EXPLICIT_INDEX,
                           VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, STREAMING_MAX_ACTIVE_TASKS);
  }

  res.createBuffer(m_transferHostBuffer, m_maxTransferBytes * STREAMING_MAX_ACTIVE_TASKS, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                   VMA_MEMORY_USAGE_CPU_ONLY, VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);

  NVVK_DBG_NAME(m_transferHostBuffer.buffer);

  m_dataInfo.blockSize         = m_blockBytes;
  m_dataInfo.memoryUsage       = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
  m_dataInfo.maxAllocatedSize  = m_maxSceneBytes;
  m_dataInfo.queueFamilies     = m_dataQueueFamilies;
  m_dataInfo.resourceAllocator = &res.m_allocator;
  m_dataInfo.usageFlags =
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;


  m_dataInfo.minAlignment = 16;


  m_dataAllocator.init(m_dataInfo);

  m_copyRegions = {};
  m_copyInfos   = {};


  m_copyRegions.reserve(std::min(config.maxGroups, 2048u));
  m_copyInfos.reserve(std::min(config.maxGroups, 2048u));
}


// 鍑芥暟锛歋treamingStorage::deinit銆傞噴鏀炬垨鍥炴敹鍓嶉潰鍒濆鍖栫殑璧勬簮锛屼繚鎸佺敓鍛藉懆鏈熸垚瀵圭鐞嗐€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氶噴鏀鹃『搴忚閬靛畧璧勬簮渚濊禆鍏崇郴锛岄伩鍏?GPU 浠嶅彲鑳借闂殑瀵硅薄琚彁鍓嶉攢姣併€?void StreamingStorage::deinit(Resources& res)
void StreamingStorage::deinit(Resources& res)
{

  res.m_allocator.destroyBuffer(m_transferHostBuffer);

  m_dataAllocator.deinit();

  m_taskCommandPool.deinit();

  m_copyInfos   = {};
  m_copyRegions = {};
}


// 鍑芥暟锛歋treamingStorage::getOperationsSize銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?size_t StreamingStorage::getOperationsSize() const
size_t StreamingStorage::getOperationsSize() const
{

  return 0;
}


// 鍑芥暟锛歋treamingStorage::getMaxDataSize銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?size_t StreamingStorage::getMaxDataSize() const
size_t StreamingStorage::getMaxDataSize() const
{
  return (m_maxSceneBytes / m_blockBytes) * m_blockBytes;
}


// 鍑芥暟锛歋treamingStorage::getNewTask銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?lodclusters::StreamingStorage::TaskInfo& StreamingStorage::getNewTask(uint32_t taskIndex)
lodclusters::StreamingStorage::TaskInfo& StreamingStorage::getNewTask(uint32_t taskIndex)
{
  TaskInfo& task  = m_taskOperations[taskIndex];
  task.baseOffset = m_maxTransferBytes * taskIndex;
  task.usedMemory = 0;


  m_copyInfos.clear();

  m_copyRegions.clear();

  return task;
}


// 鍑芥暟锛歋treamingStorage::canTransfer銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?bool StreamingStorage::canTransfer(const TaskInfo& task, size_t size) const
bool StreamingStorage::canTransfer(const TaskInfo& task, size_t size) const
{
  return task.usedMemory + size <= m_maxTransferBytes;
}


// 鍑芥暟锛歋treamingStorage::appendTransfer銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?void* StreamingStorage::appendTransfer(TaskInfo& task, const nvvk::BufferSubAllocation& dstHandle)
void* StreamingStorage::appendTransfer(TaskInfo& task, const nvvk::BufferSubAllocation& dstHandle)
{

  nvvk::BufferRange dstBinding = m_dataAllocator.subRange(dstHandle);


  assert(task.usedMemory + dstBinding.range <= m_maxTransferBytes);

  size_t transferOffset  = task.baseOffset;
  void*  transferPointer = reinterpret_cast<uint8_t*>(m_transferHostBuffer.mapping) + task.baseOffset;

  task.usedMemory += dstBinding.range;
  task.baseOffset += dstBinding.range;

  if(!m_copyInfos.empty() && m_copyInfos.back().targetBuffer == dstBinding.buffer)
  {

    VkBufferCopy& lastRegion = m_copyRegions.back();


    if(lastRegion.dstOffset + lastRegion.size == dstBinding.offset)
    {
      lastRegion.size += dstBinding.range;
      return transferPointer;
    }


  }
  else
  {

    CopyInfo task;
    task.targetBuffer = dstBinding.buffer;

    task.regionOffset = m_copyRegions.size();
    task.regionCount  = 0;

    m_copyInfos.push_back(task);
  }

  {

    VkBufferCopy region;
    region.srcOffset = transferOffset;
    region.dstOffset = dstBinding.offset;
    region.size      = dstBinding.range;

    m_copyInfos.back().regionCount++;

    m_copyRegions.push_back(region);
  }

  return transferPointer;
}


// 鍑芥暟锛歋treamingStorage::cmdUploadTask銆傚悜鍛戒护缂撳啿褰曞埗 GPU 鎿嶄綔锛屽苟渚濊禆澶栧眰璋冪敤鑰呭畨鎺掓彁浜や笌鍚屾銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳绫诲嚱鏁板彧鎻忚堪鍛戒护搴忓垪锛屼笉搴斿亣璁惧懡浠ゅ凡缁忕珛鍗虫墽琛屻€?uint32_t StreamingStorage::cmdUploadTask(VkCommandBuffer cmd)
uint32_t StreamingStorage::cmdUploadTask(VkCommandBuffer cmd)
{
  for(auto it : m_copyInfos)
  {
    vkCmdCopyBuffer(cmd, m_transferHostBuffer.buffer, it.targetBuffer, uint32_t(it.regionCount), &m_copyRegions[it.regionOffset]);
  }

  return uint32_t(m_copyRegions.size());
}


// 鍑芥暟锛歋treamingStorage::reset銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?void StreamingStorage::reset()
void StreamingStorage::reset()
{

  m_dataAllocator.deinit();
  NVVK_CHECK(m_dataAllocator.init(m_dataInfo));
}


// 鍑芥暟锛歋treamingStorage::allocate銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?bool StreamingStorage::allocate(nvvk::BufferSubAllocation& handle, GeometryGroup group, size_t sz, uint64_t& deviceAddress)
bool StreamingStorage::allocate(nvvk::BufferSubAllocation& handle, GeometryGroup group, size_t sz, uint64_t& deviceAddress)
{
  if(m_dataAllocator.subAllocate(handle, sz) == VK_SUCCESS)
  {
    deviceAddress = m_dataAllocator.subRange(handle).address;
    return true;
  }
  else
  {
    return false;
  }
}


// 鍑芥暟锛歋treamingStorage::free銆傞噴鏀炬垨鍥炴敹鍓嶉潰鍒濆鍖栫殑璧勬簮锛屼繚鎸佺敓鍛藉懆鏈熸垚瀵圭鐞嗐€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氶噴鏀鹃『搴忚閬靛畧璧勬簮渚濊禆鍏崇郴锛岄伩鍏?GPU 浠嶅彲鑳借闂殑瀵硅薄琚彁鍓嶉攢姣併€?void StreamingStorage::free(nvvk::BufferSubAllocation& handle)
void StreamingStorage::free(nvvk::BufferSubAllocation& handle)
{

  assert(handle);

  m_dataAllocator.subFree(handle);
}


// 鍑芥暟锛歋treamingStorage::getStats銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?void StreamingStorage::getStats(StreamingStats& stats) const
void StreamingStorage::getStats(StreamingStats& stats) const
{

  nvvk::BufferSubAllocator::Report report = m_dataAllocator.getReport();
  stats.reservedDataBytes                 = report.freeSize + report.reservedSize;
  stats.usedDataBytes                     = report.requestedSize;
}

}
