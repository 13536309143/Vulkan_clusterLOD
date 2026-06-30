//==============================================================================
// 鏂囦欢锛歴rc/streaming/streaming.cpp
// 妯″潡瀹氫綅锛歋ceneStreaming 涓绘祦绋嬪疄鐜帮紝澶勭悊璇锋眰瀹屾垚銆佹暟鎹笂浼犮€佸湴鍧€鏇存柊銆乤ge filter銆佸悓姝ュ拰璁＄畻 绠＄嚎銆?// 鏁版嵁娴侊細杈撳叆鏄?GPU 涓婁竴甯т骇鐢熺殑璇锋眰鍜?CPU 宸插畬鎴愪换鍔★紱杈撳嚭鏄柊椹荤暀鏁版嵁銆佸嵏杞戒慨琛ャ€佹洿鏂板悗鐨?Geometry 鍦板潃琛ㄣ€?// 鏂规硶璇存槑锛氳娴佺▼褰㈡垚闂幆锛氶亶鍘嗕骇鐢熼渶姹傦紝CPU 婊¤冻闇€姹傦紝GPU 鍦板潃琛ㄨ淇ˉ锛屼笅涓€甯ч亶鍘嗗熀浜庢柊椹荤暀闆嗗悎缁х画鍐崇瓥銆?// 姝ｇ‘鎬х害鏉燂細寮傛 transfer 涓?graphics 闃熷垪蹇呴』閫氳繃 鏃堕棿绾?semaphore 鍏宠仈锛涘け璐ユ垨閲嶅璇锋眰蹇呴』瀹夊叏閲婃斁浠诲姟绱㈠紩銆?// 娉ㄩ噴椋庢牸锛氫娇鐢ㄤ腑鏂囪В閲?CPU 渚ц涔夛紱淇濈暀蹇呰鐨?API銆佺被鍨嬪悕鍜屾暟瀛︾缉鍐欎互渚挎绱€?//==============================================================================
// 渚濊禆璇存槑锛氬紩鍏ユ湰缂栬瘧鍗曞厓闇€瑕佺殑澶栭儴搴撱€侀」鐩ā鍧楀拰鍏变韩鐫€鑹插櫒甯冨眬銆?// 渚濊禆椤哄簭閫氬父鍙嶆槧鎶借薄灞傛锛氬厛澶栭儴搴擄紝鍐嶉」鐩ā鍧楋紝鏈€鍚庝笌 GPU 鍏变韩鐨勬帴鍙ｅ畾涔夈€?#include <volk.h>
#include <volk.h>
#include <fmt/format.h>
#include "streaming.hpp"


// 瀹忛厤缃鏄庯細瀹氫箟缂栬瘧鏈熷父閲忔垨鍔熻兘寮€鍏筹紝璁?CPU 涓?GPU 鎸夊悓涓€濂楀竷灞€鍜岃矾寰勫伐浣溿€?// 瀹忓€奸€氬父浼氬奖鍝?buffer 澶у皬銆佸伐浣滅粍瑙勬ā鎴栨潯浠剁紪璇戝垎鏀紝淇敼鍚庨渶瑕佸悓鏃舵鏌?C++ 鍜岀潃鑹插櫒渚с€?#define STREAMING_DEBUG_FORCE_REQUESTS 0
#define STREAMING_DEBUG_FORCE_REQUESTS 0


// 鍛藉悕绌洪棿璇存槑锛氶檺鍒剁鍙峰彲瑙佽寖鍥达紝骞惰〃鏄庤繖浜涚被鍨嬪拰鍑芥暟灞炰簬鍚屼竴鍔熻兘鍩熴€?// 璇ヨ竟鐣屾湁鍔╀簬鍖哄垎搴旂敤灞傘€佹覆鏌撳眰銆佸満鏅眰鍜岀畻娉曞眰鐨勮亴璐ｃ€?namespace lodclusters {
namespace lodclusters {

template <class T>


// 缁撴瀯锛歄ffsetOrPointer銆傜粍缁囦竴缁勮涔夌浉鍏崇殑鏁版嵁瀛楁锛屼緵 CPU/GPU 娴佺▼鎴栨ā鍧楀唴閮ㄩ€昏緫鍏变韩銆?// 璁捐鎰忓浘锛氭妸鍚屼竴鎶借薄瀵硅薄鐨勮鏁般€佸亸绉汇€佸湴鍧€鍜岄厤缃泦涓瓨鏀撅紝闄嶄綆璺ㄥ嚱鏁颁紶閫掓椂鐨勮涔変涪澶便€?// 浣跨敤绾︽潫锛氳嫢璇ョ粨鏋勮鐫€鑹插櫒鎴栫紦瀛樻枃浠惰鍙栵紝瀛楁椤哄簭銆佸榻愭柟寮忓拰榛樿鍊奸兘灞炰簬鎺ュ彛濂戠害銆?struct OffsetOrPointer
struct OffsetOrPointer
{
  union
  {
    uint64_t offset;
    T*       pointer;
  };
};


// 鍑芥暟锛歋ceneStreaming::init銆傚垵濮嬪寲鏈ā鍧楁墍闇€鐘舵€併€佽祫婧愭垨 GPU 渚х粦瀹氥€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氬垵濮嬪寲杩囩▼寤虹珛鍚庣画闃舵鍋囧畾瀛樺湪鐨勪笉鍙橀噺锛屼緥濡傚彞鏌勬湁鏁堛€佺紦鍐插ぇ灏忚冻澶熴€佹弿杩扮宸茬粦瀹氥€?bool SceneStreaming::init(Resources* resources, const Scene* scene, const StreamingConfig& config)
bool SceneStreaming::init(Resources* resources, const Scene* scene, const StreamingConfig& config)
{

  assert(!m_resources && "no init before deinit");

  assert(resources && scene);
  Resources& res = *resources;

  m_resources = resources;
  m_scene     = scene;
  m_config    = config;
  m_shaderData = {};
  m_shaders    = {};
  m_pipelines  = {};
  m_lastUpdateIndex         = 0;
  m_frameIndex              = 1;
  m_operationsSize          = 0;
  m_persistentGeometrySize  = 0;
  m_stats                   = {};
  m_config.maxGroups = std::max(m_config.maxGroups, uint32_t(scene->getActiveGeometryCount()));
  if(m_config.maxClusters == 0)
  {
    m_config.maxClusters = config.maxGroups * scene->m_config.clusterGroupSize;
  }
  m_config.maxClusters =
      std::max(m_config.maxClusters, uint32_t(scene->getActiveGeometryCount()) * scene->m_config.clusterGroupSize);

  m_stats.maxLoadCount     = m_config.maxPerFrameLoadRequests;
  m_stats.maxUnloadCount   = m_config.maxPerFrameUnloadRequests;
  m_stats.maxGroups        = m_config.maxGroups;
  m_stats.maxClusters      = m_config.maxClusters;
  m_stats.maxTransferBytes = m_config.maxTransferMegaBytes * 1024 * 1024;


  {
    nvvk::DescriptorBindings bindings;

    bindings.addBinding(BINDINGS_FRAME_UBO, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT);

    bindings.addBinding(BINDINGS_READBACK_SSBO, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT);

    bindings.addBinding(BINDINGS_GEOMETRIES_SSBO, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT);

    bindings.addBinding(BINDINGS_SCENEBUILDING_SSBO, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT);

    bindings.addBinding(BINDINGS_SCENEBUILDING_UBO, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT);

    bindings.addBinding(BINDINGS_STREAMING_SSBO, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT);

    bindings.addBinding(BINDINGS_STREAMING_UBO, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT);

    m_dsetPack.init(bindings, res.m_device);

    nvvk::createPipelineLayout(res.m_device, &m_pipelineLayout, {m_dsetPack.getLayout()},
                               {{VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t)}});
  }

  if(!initShadersAndPipelines())
  {

    m_dsetPack.deinit();
    return false;
  }


  uint32_t groupCountAlignment = std::max(STREAM_AGEFILTER_GROUPS_WORKGROUP, STREAM_UPDATE_SCENE_WORKGROUP);
  uint32_t clusterCountAlignment = STREAM_UPDATE_SCENE_WORKGROUP;


  m_requestsTaskQueue = {};
  m_updatesTaskQueue  = {};
  m_storageTaskQueue  = {};


  m_requests.init(res, m_config, groupCountAlignment, clusterCountAlignment);

  m_resident.init(res, m_config, groupCountAlignment, clusterCountAlignment);
  m_updates.init(res, m_config, groupCountAlignment, clusterCountAlignment);

  m_storage.init(res, m_config);


  m_stats.maxDataBytes = m_storage.getMaxDataSize();

  m_operationsSize += logMemoryUsage(m_requests.getOperationsSize(), "operations", "stream requests");
  m_operationsSize += logMemoryUsage(m_resident.getOperationsSize(), "operations", "stream resident");
  m_operationsSize += logMemoryUsage(m_updates.getOperationsSize(), "operations", "stream updates");
  m_operationsSize += logMemoryUsage(m_storage.getOperationsSize(), "operations", "stream storage");

  res.createBuffer(m_shaderBuffer, sizeof(shaderio::SceneStreaming),
                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
                       | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

  NVVK_DBG_NAME(m_shaderBuffer.buffer);


  m_operationsSize += logMemoryUsage(m_shaderBuffer.bufferSize, "operations", "stream shaderio");


  initGeometries(res, scene);

  return true;
}


// 鍑芥暟锛歋ceneStreaming::updateBindings銆傛牴鎹渶鏂扮姸鎬佸埛鏂扮紦瀛樻暟鎹€丟PU 鍦板潃銆佹弿杩扮鎴栫粺璁′俊鎭€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氭洿鏂板嚱鏁拌礋璐ｆ妸鈥滄棫鐘舵€佲€濇帹杩涘埌鈥滃綋鍓嶇姸鎬佲€濓紝鍥犳瑕侀伩鍏嶉儴鍒嗘洿鏂伴€犳垚 CPU/GPU 瑙嗗浘涓嶄竴鑷淬€?void SceneStreaming::updateBindings(const nvvk::Buffer& sceneBuildingBuffer)
void SceneStreaming::updateBindings(const nvvk::Buffer& sceneBuildingBuffer)
{
  nvvk::WriteSetContainer writeSets;
  writeSets.append(m_dsetPack.makeWrite(BINDINGS_FRAME_UBO), m_resources->m_commonBuffers.frameConstants);
  writeSets.append(m_dsetPack.makeWrite(BINDINGS_READBACK_SSBO), m_resources->m_commonBuffers.readBack);
  writeSets.append(m_dsetPack.makeWrite(BINDINGS_GEOMETRIES_SSBO), m_shaderGeometriesBuffer);
  writeSets.append(m_dsetPack.makeWrite(BINDINGS_SCENEBUILDING_SSBO), sceneBuildingBuffer);
  writeSets.append(m_dsetPack.makeWrite(BINDINGS_SCENEBUILDING_UBO), sceneBuildingBuffer);
  writeSets.append(m_dsetPack.makeWrite(BINDINGS_STREAMING_SSBO), m_shaderBuffer);
  writeSets.append(m_dsetPack.makeWrite(BINDINGS_STREAMING_UBO), m_shaderBuffer);
  vkUpdateDescriptorSets(m_resources->m_device, writeSets.size(), writeSets.data(), 0, nullptr);
}


// 鍑芥暟锛歋ceneStreaming::resetGeometryGroupAddresses銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?void SceneStreaming::resetGeometryGroupAddresses(Resources::BatchedUploader& uploader)
void SceneStreaming::resetGeometryGroupAddresses(Resources::BatchedUploader& uploader)
{


  for(size_t geometryIndex = 0; geometryIndex < m_scene->getActiveGeometryCount(); geometryIndex++)
  {
    SceneStreaming::PersistentGeometry& persistentGeometry = m_persistentGeometries[geometryIndex];
    shaderio::Geometry&                 shaderGeometry     = m_shaderGeometries[geometryIndex];

    const Scene::GeometryView&          sceneGeometry      = m_scene->getActiveGeometry(geometryIndex);


    shaderio::LodLevel lastLodLevel = sceneGeometry.lodLevels.back();

    uint64_t* groupAddresses = uploader.uploadBuffer(persistentGeometry.groupAddresses, (uint64_t*)nullptr);
    for(uint32_t groupIndex = 0; groupIndex < lastLodLevel.groupOffset; groupIndex++)
    {
      groupAddresses[groupIndex] = STREAMING_INVALID_ADDRESS_START;
    }

    groupAddresses[lastLodLevel.groupOffset] = persistentGeometry.lowDetailGroupsData.address;


    uint32_t maxLodLevel = persistentGeometry.lodLevelsCount - 1;
    for(uint32_t i = 0; i < maxLodLevel; i++)
    {
      persistentGeometry.lodLoadedGroupsCount[i] = 0;
    }
    persistentGeometry.lodLoadedGroupsCount[maxLodLevel] = 1;
  }
}


// 鍑芥暟锛歋ceneStreaming::initGeometries銆傚垵濮嬪寲鏈ā鍧楁墍闇€鐘舵€併€佽祫婧愭垨 GPU 渚х粦瀹氥€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氬垵濮嬪寲杩囩▼寤虹珛鍚庣画闃舵鍋囧畾瀛樺湪鐨勪笉鍙橀噺锛屼緥濡傚彞鏌勬湁鏁堛€佺紦鍐插ぇ灏忚冻澶熴€佹弿杩扮宸茬粦瀹氥€?void SceneStreaming::initGeometries(Resources& res, const Scene* scene)
void SceneStreaming::initGeometries(Resources& res, const Scene* scene)
{


  // 鍑芥暟锛歶ploader銆備粠鏂囦欢銆佺紦瀛樸€丟PU 缂撳啿鎴栧叡浜竷灞€涓鍙栨暟鎹苟杞崲涓烘湰妯″潡鏍煎紡銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍙栬矾寰勯渶瑕佹牎楠岃緭鍏ュ悎娉曟€э紝骞舵妸澶栭儴鏍煎紡鐨勪笉纭畾鎬ц浆鍖栦负鍐呴儴纭畾甯冨眬銆?  Resources::BatchedUploader uploader(res);
  Resources::BatchedUploader uploader(res);

  m_shaderGeometries.resize(scene->getActiveGeometryCount());
  m_persistentGeometries.resize(scene->getActiveGeometryCount());

  uint32_t instancesOffset = 0;
  for(size_t geometryIndex = 0; geometryIndex < scene->getActiveGeometryCount(); geometryIndex++)
  {
    shaderio::Geometry&                 shaderGeometry     = m_shaderGeometries[geometryIndex];
    SceneStreaming::PersistentGeometry& persistentGeometry = m_persistentGeometries[geometryIndex];

    const Scene::GeometryView&          sceneGeometry      = m_scene->getActiveGeometry(geometryIndex);


    size_t numGroups = sceneGeometry.groupInfos.size();

    res.createBufferTyped(persistentGeometry.groupAddresses, numGroups, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    NVVK_DBG_NAME(persistentGeometry.groupAddresses.buffer);


    size_t numNodes = sceneGeometry.lodNodes.size();

    res.createBufferTyped(persistentGeometry.nodes, numNodes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    res.createBufferTyped(persistentGeometry.nodeBboxes, numNodes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    NVVK_DBG_NAME(persistentGeometry.nodes.buffer);

    NVVK_DBG_NAME(persistentGeometry.nodeBboxes.buffer);

    uint32_t numLodLevels = sceneGeometry.lodLevelsCount;

    res.createBufferTyped(persistentGeometry.lodLevels, numLodLevels, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    NVVK_DBG_NAME(persistentGeometry.lodLevels.buffer);

    m_persistentGeometrySize += persistentGeometry.groupAddresses.bufferSize;
    m_persistentGeometrySize += persistentGeometry.nodes.bufferSize;
    m_persistentGeometrySize += persistentGeometry.nodeBboxes.bufferSize;


    shaderGeometry                         = {};
    shaderGeometry.bbox                    = sceneGeometry.bbox;
    shaderGeometry.nodes                   = persistentGeometry.nodes.address;
    shaderGeometry.nodeBboxes              = persistentGeometry.nodeBboxes.address;
    shaderGeometry.streamingGroupAddresses = persistentGeometry.groupAddresses.address;
    shaderGeometry.lodLevelsCount          = numLodLevels;
    shaderGeometry.lodLevels               = persistentGeometry.lodLevels.address;

    shaderGeometry.instancesCount          = sceneGeometry.instanceReferenceCount * scene->getGeometryInstanceFactor();
    shaderGeometry.instancesOffset         = instancesOffset;

    instancesOffset += shaderGeometry.instancesCount;

    persistentGeometry.lodLevelsCount = numLodLevels;
    for(uint32_t i = 0; i < numLodLevels; i++)
    {
      persistentGeometry.lodGroupsCount[i] = sceneGeometry.lodLevels[i].groupCount;
    }


    uploader.uploadBuffer(persistentGeometry.nodes, sceneGeometry.lodNodes.data());
    uploader.uploadBuffer(persistentGeometry.nodeBboxes, sceneGeometry.lodNodeBboxes.data());
    uploader.uploadBuffer(persistentGeometry.lodLevels, sceneGeometry.lodLevels.data());


    shaderio::LodLevel     lastLodLevel = sceneGeometry.lodLevels.back();
    const Scene::GroupInfo groupInfo    = sceneGeometry.groupInfos[lastLodLevel.groupOffset];


    // 鍑芥暟锛歡roupView銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?    // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?    // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?    Scene::GroupView       groupView(sceneGeometry.groupData, groupInfo);
    Scene::GroupView       groupView(sceneGeometry.groupData, groupInfo);

    assert(groupInfo.clusterCount == 1);

    GeometryGroup geometryGroup     = {uint32_t(geometryIndex), lastLodLevel.groupOffset};
    uint32_t      lastClustersCount = groupInfo.clusterCount;

    uint64_t      lastGroupSize     = groupInfo.getDeviceSize();


    res.createBuffer(persistentGeometry.lowDetailGroupsData, lastGroupSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    NVVK_DBG_NAME(persistentGeometry.lowDetailGroupsData.buffer);
    m_persistentGeometrySize += persistentGeometry.lowDetailGroupsData.bufferSize;


    assert(lastClustersCount <= 0xFFFFFFFF);
    assert(m_resident.canAllocateGroup(uint32_t(lastClustersCount)));


    StreamingResident::Group* rgroup = m_resident.addGroup(geometryGroup, lastClustersCount);
    rgroup->deviceAddress            = persistentGeometry.lowDetailGroupsData.address;
    rgroup->lodLevel                 = groupInfo.lodLevel;

    persistentGeometry.lodLoadedGroupsCount[groupInfo.lodLevel] = 1;


    void* loGroupData = uploader.uploadBuffer(persistentGeometry.lowDetailGroupsData, (void*)nullptr);

    Scene::fillGroupRuntimeData(groupInfo, groupView, geometryGroup.groupID, rgroup->groupResidentID,
                                rgroup->clusterResidentID, loGroupData, persistentGeometry.lowDetailGroupsData.bufferSize);

    shaderGeometry.lowDetailClusterID = rgroup->clusterResidentID;
    shaderGeometry.lowDetailTriangles = groupInfo.triangleCount;
  }


  resetGeometryGroupAddresses(uploader);

  res.createBufferTyped(m_shaderGeometriesBuffer, scene->getActiveGeometryCount(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

  NVVK_DBG_NAME(m_shaderGeometriesBuffer.buffer);

  m_operationsSize += logMemoryUsage(m_shaderGeometriesBuffer.bufferSize, "operations", "stream geo buffer");

  uploader.uploadBuffer(m_shaderGeometriesBuffer, m_shaderGeometries.data());


  m_resident.uploadInitialState(uploader, m_shaderData.resident);


  uploader.flush();
}

void SceneStreaming::cmdBeginFrame(VkCommandBuffer         cmd,
                                   QueueState&             cmdQueueState,
                                   QueueState&             asyncQueueState,
                                   const FrameSettings&    settings,
                                   nvvk::ProfilerGpuTimer& profiler)
{


  auto     timerSection = profiler.cmdFrameSection(cmd, "Stream Begin");
  VkDevice device       = m_resources->m_device;


  const bool ensureAcquisition = true;


  uint32_t updateTaskCount = 0;
  while(m_updatesTaskQueue.canPop(device, ensureAcquisition))
  {


    uint32_t popUpdateIndex = m_updatesTaskQueue.pop();


    const StreamingUpdates::TaskInfo& update = m_updates.getCompletedTask(popUpdateIndex);
    for(uint32_t g = 0; g < update.unloadCount; g++)
    {

      m_storage.free(update.unloadHandles[g]);
    }


    m_updatesTaskQueue.releaseTaskIndex(popUpdateIndex);


    if(++updateTaskCount >= 16)
    {
      updateTaskCount = 0;

      break;
    }
  }


  uint32_t pushUpdateIndex = INVALID_TASK_INDEX;


  if(m_storageTaskQueue.canPop(device, ensureAcquisition))
  {


    uint32_t dependentIndex  = INVALID_TASK_INDEX;

    uint32_t popStorageIndex = m_storageTaskQueue.popWithDependent(dependentIndex);

    m_storageTaskQueue.releaseTaskIndex(popStorageIndex);


    if(dependentIndex != INVALID_TASK_INDEX)
    {
      pushUpdateIndex = dependentIndex;
    }
  }

  bool isImmediateUpdate = false;


  if(m_requestsTaskQueue.canPop(device, ensureAcquisition))
  {

    uint32_t popRequestIndex = m_requestsTaskQueue.pop();

#if 1


    while(m_requestsTaskQueue.canPop(device, false))
    {


      m_requestsTaskQueue.releaseTaskIndex(popRequestIndex);


      popRequestIndex = m_requestsTaskQueue.pop();
    }
#endif


    uint32_t dependentIndex = handleCompletedRequest(cmd, cmdQueueState, asyncQueueState, settings, popRequestIndex);

    if(dependentIndex != INVALID_TASK_INDEX)
    {


      assert(pushUpdateIndex == INVALID_TASK_INDEX);

      pushUpdateIndex   = dependentIndex;
      isImmediateUpdate = true;
    }
  }


  if(pushUpdateIndex != INVALID_TASK_INDEX)
  {


    m_resident.applyTask(m_shaderData.resident, pushUpdateIndex, m_frameIndex);

    m_updates.applyTask(m_shaderData.update, pushUpdateIndex, m_frameIndex);


    m_updatesTaskQueue.push(pushUpdateIndex, cmdQueueState.getCurrentState());

    m_lastUpdateIndex = pushUpdateIndex;
  }
  else
  {

    m_shaderData.update.patchGroupsCount         = 0;
    m_shaderData.update.patchUnloadGroupsCount   = 0;
    m_shaderData.update.loadActiveGroupsOffset   = 0;
    m_shaderData.update.loadActiveClustersOffset = 0;
    m_shaderData.update.taskIndex                = INVALID_TASK_INDEX;
    m_shaderData.update.frameIndex               = m_frameIndex;
  }


  {


    uint32_t pushRequestIndex = m_requestsTaskQueue.acquireTaskIndex();


    assert(pushRequestIndex != INVALID_TASK_INDEX);


    m_requests.applyTask(m_shaderData.request, pushRequestIndex, m_frameIndex);
  }

  m_shaderData.frameIndex               = m_frameIndex;
  m_shaderData.ageThreshold             = settings.ageThreshold;


  vkCmdUpdateBuffer(cmd, m_shaderBuffer.buffer, 0, sizeof(m_shaderData), &m_shaderData);
}

uint32_t SceneStreaming::handleCompletedRequest(VkCommandBuffer      cmd,
                                                QueueState&          cmdQueueState,
                                                QueueState&          asyncQueueState,
                                                const FrameSettings& settings,
                                                uint32_t             popRequestIndex)
{


  const StreamingRequests::TaskInfo& request = m_requests.getCompletedTask(popRequestIndex);


  uint32_t loadCount   = std::min(request.shaderData->maxLoads, request.shaderData->loadCounter);

  uint32_t unloadCount = std::min(request.shaderData->maxUnloads, request.shaderData->unloadCounter);


#if !STREAMING_DEBUG_FORCE_REQUESTS
  if((!loadCount && !unloadCount) || !m_debugFrameLimit)
  {


    m_requestsTaskQueue.releaseTaskIndex(popRequestIndex);
    return INVALID_TASK_INDEX;
  }
#endif


  if(m_debugFrameLimit > 0)
    m_debugFrameLimit--;


  uint32_t pushStorageIndex = m_storageTaskQueue.acquireTaskIndex();

  uint32_t pushUpdateIndex  = m_updatesTaskQueue.acquireTaskIndex();


  if(pushStorageIndex == INVALID_TASK_INDEX || pushUpdateIndex == INVALID_TASK_INDEX)
  {

    if(pushStorageIndex != INVALID_TASK_INDEX)
    {

      m_storageTaskQueue.releaseTaskIndex(pushStorageIndex);
    }
    if(pushUpdateIndex != INVALID_TASK_INDEX)
    {

      m_updatesTaskQueue.releaseTaskIndex(pushUpdateIndex);
    }

    m_requestsTaskQueue.releaseTaskIndex(popRequestIndex);
    return INVALID_TASK_INDEX;
  }


  StreamingStorage::TaskInfo& storageTask = m_storage.getNewTask(pushStorageIndex);

  StreamingUpdates::TaskInfo& updateTask  = m_updates.getNewTask(pushUpdateIndex);


  for(uint32_t g = 0; g < unloadCount; g++)
  {
    GeometryGroup geometryGroup = request.unloadGeometryGroups[g];

    assert(geometryGroup.geometryID < m_scene->getActiveGeometryCount());
    assert(geometryGroup.groupID < m_scene->getActiveGeometry(geometryGroup.geometryID).totalClustersCount);


    const StreamingResident::Group* group = m_resident.findGroup(geometryGroup);
    if(!group)
    {


      continue;
    }


    uint32_t                  unloadIndex = updateTask.unloadCount++;
    shaderio::StreamingPatch& patch       = updateTask.unloadPatches[unloadIndex];
    patch.geometryID                      = geometryGroup.geometryID;
    patch.groupID                         = geometryGroup.groupID;
    patch.groupAddress                    = STREAMING_INVALID_ADDRESS_START;


    assert(group->storageHandle);
    updateTask.unloadHandles[unloadIndex] = group->storageHandle;


    assert(m_persistentGeometries[geometryGroup.geometryID].lodLoadedGroupsCount[group->lodLevel] > 0);
    m_persistentGeometries[geometryGroup.geometryID].lodLoadedGroupsCount[group->lodLevel]--;


    m_resident.removeGroup(group->groupResidentID);


  }


  updateTask.loadActiveGroupsOffset   = m_resident.getLoadActiveGroupsOffset();

  updateTask.loadActiveClustersOffset = m_resident.getLoadActiveClustersOffset();

  uint64_t transferBytes = 0;

  m_stats.couldNotTransfer      = 0;
  m_stats.couldNotAllocateGroup = 0;
  m_stats.couldNotStore         = 0;
  m_stats.uncompletedLoadCount  = 0;


  uint32_t processedLoads = 0;
  for(uint32_t g = 0; g < loadCount; g++)
  {
    GeometryGroup geometryGroup = request.loadGeometryGroups[g];

    assert(geometryGroup.geometryID < m_scene->getActiveGeometryCount());
    assert(geometryGroup.groupID < m_scene->getActiveGeometry(geometryGroup.geometryID).totalClustersCount);

    if(m_resident.findGroup(geometryGroup))
    {


      continue;
    }


    const Scene::GeometryView& sceneGeometry = m_scene->getActiveGeometry(geometryGroup.geometryID);


    const Scene::GroupInfo groupInfo       = sceneGeometry.groupInfos[geometryGroup.groupID];
    uint32_t               clusterCount    = groupInfo.clusterCount;

    uint64_t               groupDeviceSize = groupInfo.getDeviceSize();

    uint64_t                  deviceAddress;
    nvvk::BufferSubAllocation storageHandle;


    if(!m_storage.canTransfer(storageTask, groupDeviceSize))
    {
      m_stats.couldNotTransfer++;
      m_stats.uncompletedLoadCount++;
      continue;
    }


    bool canStore         = m_storage.allocate(storageHandle, geometryGroup, groupDeviceSize, deviceAddress);

    bool canAllocateGroup = m_resident.canAllocateGroup(clusterCount);


    if(!canStore || !canAllocateGroup)
    {
      m_stats.couldNotAllocateGroup += (!canAllocateGroup);
      m_stats.couldNotStore += (!canStore);

      if(canStore)
      {


        m_storage.free(storageHandle);
      }

      if(clusterCount < 8)
      {
        m_stats.uncompletedLoadCount += loadCount - g;
        break;
      }
      else
      {
        m_stats.uncompletedLoadCount++;
        continue;
      }
    }

    processedLoads++;


    StreamingResident::Group* residentGroup = m_resident.addGroup(geometryGroup, clusterCount);
    residentGroup->storageHandle            = storageHandle;
    residentGroup->deviceAddress            = deviceAddress;
    residentGroup->lodLevel                 = groupInfo.lodLevel;

    void* groupData                         = m_storage.appendTransfer(storageTask, residentGroup->storageHandle);


    assert(deviceAddress % 16 == 0);

    {


      // 鍑芥暟锛歡roupView銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?      // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?      // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?      Scene::GroupView groupView(sceneGeometry.groupData, groupInfo);
      Scene::GroupView groupView(sceneGeometry.groupData, groupInfo);
      if(groupInfo.uncompressedSizeBytes)
      {

        Scene::decompressGroup(groupInfo, groupView, groupData, groupDeviceSize);
      }
      else
      {


        memcpy(groupData, groupView.raw, groupView.rawSize);
      }
    }

    m_persistentGeometries[geometryGroup.geometryID].lodLoadedGroupsCount[groupInfo.lodLevel]++;


    shaderio::StreamingPatch& patch = updateTask.loadPatches[updateTask.loadCount++];
    patch.geometryID                = geometryGroup.geometryID;
    patch.groupID                   = geometryGroup.groupID;
    patch.groupAddress              = deviceAddress;
    patch.groupResidentID           = residentGroup->groupResidentID;
    patch.clusterResidentID         = residentGroup->clusterResidentID;
    patch.clusterCount              = groupInfo.clusterCount;
    patch.lodLevel                  = groupInfo.lodLevel;


    transferBytes += groupInfo.sizeBytes;
  }

#if !STREAMING_DEBUG_FORCE_REQUESTS
  if(updateTask.loadCount == 0 && updateTask.unloadCount == 0)
  {


    m_requestsTaskQueue.releaseTaskIndex(popRequestIndex);

    m_updatesTaskQueue.releaseTaskIndex(pushUpdateIndex);

    m_storageTaskQueue.releaseTaskIndex(pushStorageIndex);
    return INVALID_TASK_INDEX;
  }
#endif

  if(m_config.useAsyncTransfer)
  {


    NVVK_CHECK(m_storage.m_taskCommandPool.acquireCommandBuffer(pushStorageIndex, cmd));
    VkCommandBufferBeginInfo cmdInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
    };


    vkBeginCommandBuffer(cmd, &cmdInfo);
  }

  uint32_t transferCount = 0;


  transferBytes += m_updates.cmdUploadTask(cmd, pushUpdateIndex);

  transferBytes += m_resident.cmdUploadTask(cmd, pushUpdateIndex);

  transferCount += m_storage.cmdUploadTask(cmd);
  transferCount += 2;


  if(processedLoads > 0) {

    m_stats.transferBytes = transferBytes;
    m_stats.transferCount = transferCount;
    m_stats.loadCount = updateTask.loadCount;
  }
  if(updateTask.unloadCount > 0) {
    m_stats.unloadCount = updateTask.unloadCount;
  }


  bool useDecoupledUpdate = m_config.useAsyncTransfer && m_config.useDecoupledAsyncTransfer;

  nvvk::SemaphoreState storageSemaphoreState =

      m_config.useAsyncTransfer ? asyncQueueState.getCurrentState() : cmdQueueState.getCurrentState();

  if(m_config.useAsyncTransfer)
  {

    vkEndCommandBuffer(cmd);

    if(!m_config.useDecoupledAsyncTransfer)
    {


      VkSemaphoreSubmitInfo semWaitInfo = asyncQueueState.getWaitSubmit(VK_PIPELINE_STAGE_2_TRANSFER_BIT);


      cmdQueueState.m_pendingWaits.push_back(semWaitInfo);
    }


    VkSemaphoreSubmitInfo     semSubmitInfo = asyncQueueState.advanceSignalSubmit(VK_PIPELINE_STAGE_2_TRANSFER_BIT);
    VkCommandBufferSubmitInfo cmdBufInfo    = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
    cmdBufInfo.commandBuffer                = cmd;


    VkSubmitInfo2 submits            = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR};
    submits.pCommandBufferInfos      = &cmdBufInfo;
    submits.commandBufferInfoCount   = 1;
    submits.pSignalSemaphoreInfos    = &semSubmitInfo;
    submits.signalSemaphoreInfoCount = 1;


    vkQueueSubmit2(asyncQueueState.m_queue, 1, &submits, nullptr);
  }


  m_requestsTaskQueue.releaseTaskIndex(popRequestIndex);


  m_storageTaskQueue.push(pushStorageIndex, storageSemaphoreState, useDecoupledUpdate ? pushUpdateIndex : INVALID_TASK_INDEX);


  return useDecoupledUpdate ? INVALID_TASK_INDEX : pushUpdateIndex;
}


// 鍑芥暟锛歡etWorkGroupCount銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?static uint32_t getWorkGroupCount(uint32_t numThreads, uint32_t workGroupSize)
static uint32_t getWorkGroupCount(uint32_t numThreads, uint32_t workGroupSize)
{

  return (numThreads + workGroupSize - 1) / workGroupSize;
}


// 鍑芥暟锛歋ceneStreaming::cmdPreTraversal銆傚悜鍛戒护缂撳啿褰曞埗 GPU 鎿嶄綔锛屽苟渚濊禆澶栧眰璋冪敤鑰呭畨鎺掓彁浜や笌鍚屾銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳绫诲嚱鏁板彧鎻忚堪鍛戒护搴忓垪锛屼笉搴斿亣璁惧懡浠ゅ凡缁忕珛鍗虫墽琛屻€?void SceneStreaming::cmdPreTraversal(VkCommandBuffer cmd, nvvk::ProfilerGpuTimer& profiler)
void SceneStreaming::cmdPreTraversal(VkCommandBuffer cmd, nvvk::ProfilerGpuTimer& profiler)
{
  Resources& res = *m_resources;


  auto timerSection = profiler.cmdFrameSection(cmd, "Stream Pre Traversal");


  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1, m_dsetPack.getSetPtr(), 0, nullptr);


  if(m_shaderData.update.patchGroupsCount)
  {

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelines.computeUpdateSceneRaster);

    res.cmdLinearDispatch(cmd, getWorkGroupCount(m_shaderData.update.patchGroupsCount, STREAM_UPDATE_SCENE_WORKGROUP));


    m_resident.cmdRunTask(cmd, m_shaderData.update.taskIndex);
  }


}


// 鍑芥暟锛歋ceneStreaming::cmdPostTraversal銆傚悜鍛戒护缂撳啿褰曞埗 GPU 鎿嶄綔锛屽苟渚濊禆澶栧眰璋冪敤鑰呭畨鎺掓彁浜や笌鍚屾銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳绫诲嚱鏁板彧鎻忚堪鍛戒护搴忓垪锛屼笉搴斿亣璁惧懡浠ゅ凡缁忕珛鍗虫墽琛屻€?void SceneStreaming::cmdPostTraversal(VkCommandBuffer cmd, bool runAgeFilter, nvvk::ProfilerGpuTimer& profiler)
void SceneStreaming::cmdPostTraversal(VkCommandBuffer cmd, bool runAgeFilter, nvvk::ProfilerGpuTimer& profiler)
{
  Resources& res = *m_resources;


  auto timerSection = profiler.cmdFrameSection(cmd, "Stream Post Traversal");

  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1, m_dsetPack.getSetPtr(), 0, nullptr);

  if(m_shaderData.resident.activeGroupsCount && runAgeFilter)
  {


    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelines.computeAgeFilterGroups);
    res.cmdLinearDispatch(cmd, getWorkGroupCount(m_shaderData.resident.activeGroupsCount, STREAM_AGEFILTER_GROUPS_WORKGROUP));
  }


}


// 鍑芥暟锛歋ceneStreaming::cmdEndFrame銆傚悜鍛戒护缂撳啿褰曞埗 GPU 鎿嶄綔锛屽苟渚濊禆澶栧眰璋冪敤鑰呭畨鎺掓彁浜や笌鍚屾銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳绫诲嚱鏁板彧鎻忚堪鍛戒护搴忓垪锛屼笉搴斿亣璁惧懡浠ゅ凡缁忕珛鍗虫墽琛屻€?void SceneStreaming::cmdEndFrame(VkCommandBuffer cmd, QueueState& cmdQueueState, nvvk::ProfilerGpuTimer& profiler)
void SceneStreaming::cmdEndFrame(VkCommandBuffer cmd, QueueState& cmdQueueState, nvvk::ProfilerGpuTimer& profiler)
{


  auto timerSection = profiler.cmdFrameSection(cmd, "Stream End");

  m_requests.cmdRunTask(cmd, m_shaderData.request, m_shaderBuffer.buffer, offsetof(shaderio::SceneStreaming, request));

  m_requestsTaskQueue.push(m_shaderData.request.taskIndex, cmdQueueState.getCurrentState());

  m_frameIndex++;


  size_t geoSize = getGeometrySize(false);
  if(geoSize > m_peakGeometrySize)
  {
    m_peakGeometrySize = geoSize;
    m_peakFrameIndex   = m_frameIndex;
  }
  else if(m_frameIndex == m_peakFrameIndex + 2)
  {

    LOGI("streaming: geometry peak frame %d\n", m_peakFrameIndex);
  }
}


// 鍑芥暟锛歋ceneStreaming::getStats銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?void SceneStreaming::getStats(StreamingStats& stats) const
void SceneStreaming::getStats(StreamingStats& stats) const
{
  stats = m_stats;


  m_storage.getStats(stats);

  m_resident.getStats(stats);
  stats.persistentDataBytes = m_persistentGeometrySize;
}


// 鍑芥暟锛歋ceneStreaming::getGeometrySize銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?size_t SceneStreaming::getGeometrySize(bool reserved) const
size_t SceneStreaming::getGeometrySize(bool reserved) const
{
  StreamingStats stats;

  getStats(stats);

  if(reserved)
  {
    return m_persistentGeometrySize + stats.reservedDataBytes;
  }
  else
  {
    return m_persistentGeometrySize + stats.usedDataBytes;
  }
}


// 鍑芥暟锛歋ceneStreaming::deinit銆傞噴鏀炬垨鍥炴敹鍓嶉潰鍒濆鍖栫殑璧勬簮锛屼繚鎸佺敓鍛藉懆鏈熸垚瀵圭鐞嗐€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氶噴鏀鹃『搴忚閬靛畧璧勬簮渚濊禆鍏崇郴锛岄伩鍏?GPU 浠嶅彲鑳借闂殑瀵硅薄琚彁鍓嶉攢姣併€?void SceneStreaming::deinit()
void SceneStreaming::deinit()
{
  if(!m_resources)
    return;

  Resources& res = *m_resources;


  deinitShadersAndPipelines();

  m_dsetPack.deinit();

  vkDestroyPipelineLayout(res.m_device, m_pipelineLayout, nullptr);


  m_resident.deinit(res);

  m_storage.deinit(res);

  m_updates.deinit(res);

  m_requests.deinit(res);

  for(auto& it : m_persistentGeometries)
  {

    res.m_allocator.destroyBuffer(it.groupAddresses);

    res.m_allocator.destroyBuffer(it.nodeBboxes);

    res.m_allocator.destroyBuffer(it.nodes);

    res.m_allocator.destroyBuffer(it.lodLevels);

    res.m_allocator.destroyBuffer(it.lowDetailGroupsData);
  }

  m_persistentGeometries.clear();


  res.m_allocator.destroyBuffer(m_shaderGeometriesBuffer);

  res.m_allocator.destroyBuffer(m_shaderBuffer);

  m_resources = nullptr;
  m_scene     = nullptr;
}


// 鍑芥暟锛歋ceneStreaming::reset銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?void SceneStreaming::reset()
void SceneStreaming::reset()
{
  Resources& res = *m_resources;


  vkDeviceWaitIdle(res.m_device);

  m_debugFrameLimit  = s_defaultDebugFrameLimit;
  m_peakFrameIndex   = ~0;
  m_peakGeometrySize = 0;

  m_requestsTaskQueue = {};
  m_storageTaskQueue  = {};
  m_updatesTaskQueue  = {};


  m_resident.reset(m_shaderData.resident);

  m_updates.reset();


  m_storage.reset();


  m_frameIndex = 1;


  // 鍑芥暟锛歶ploader銆備粠鏂囦欢銆佺紦瀛樸€丟PU 缂撳啿鎴栧叡浜竷灞€涓鍙栨暟鎹苟杞崲涓烘湰妯″潡鏍煎紡銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍙栬矾寰勯渶瑕佹牎楠岃緭鍏ュ悎娉曟€э紝骞舵妸澶栭儴鏍煎紡鐨勪笉纭畾鎬ц浆鍖栦负鍐呴儴纭畾甯冨眬銆?  Resources::BatchedUploader uploader(res);
  Resources::BatchedUploader uploader(res);

  resetGeometryGroupAddresses(uploader);

  uploader.flush();
}


// 鍑芥暟锛歋ceneStreaming::initShadersAndPipelines銆傚垵濮嬪寲鏈ā鍧楁墍闇€鐘舵€併€佽祫婧愭垨 GPU 渚х粦瀹氥€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氬垵濮嬪寲杩囩▼寤虹珛鍚庣画闃舵鍋囧畾瀛樺湪鐨勪笉鍙橀噺锛屼緥濡傚彞鏌勬湁鏁堛€佺紦鍐插ぇ灏忚冻澶熴€佹弿杩扮宸茬粦瀹氥€?bool SceneStreaming::initShadersAndPipelines()
bool SceneStreaming::initShadersAndPipelines()
{
  Resources& res = *m_resources;


  shaderc::CompileOptions options = res.makeCompilerOptions();
  options.AddMacroDefinition("SUBGROUP_SIZE", fmt::format("{}", res.m_physicalDeviceInfo.properties11.subgroupSize));
  options.AddMacroDefinition("USE_16BIT_DISPATCH", fmt::format("{}", res.m_use16bitDispatch ? 1 : 0));

  shaderc::CompileOptions optionsRaster = options;

  optionsRaster.AddMacroDefinition("TARGETS_RASTERIZATION", "1");


  res.compileShader(m_shaders.computeAgeFilterGroups, VK_SHADER_STAGE_COMPUTE_BIT, "streaming/stream_agefilter_groups.comp.glsl", &options);

  res.compileShader(m_shaders.computeUpdateSceneRaster, VK_SHADER_STAGE_COMPUTE_BIT, "streaming/stream_update_scene.comp.glsl", &optionsRaster);

  if(!res.verifyShaders(m_shaders))
  {
    return false;
  }

  {
    VkComputePipelineCreateInfo compInfo   = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    VkShaderModuleCreateInfo    shaderInfo = {};
    compInfo.stage                         = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    compInfo.stage.stage                   = VK_SHADER_STAGE_COMPUTE_BIT;
    compInfo.stage.pName                   = "main";
    compInfo.stage.pNext                   = &shaderInfo;
    compInfo.layout                        = m_pipelineLayout;


    shaderInfo = nvvkglsl::GlslCompiler::makeShaderModuleCreateInfo(m_shaders.computeAgeFilterGroups);

    vkCreateComputePipelines(res.m_device, nullptr, 1, &compInfo, nullptr, &m_pipelines.computeAgeFilterGroups);


    shaderInfo = nvvkglsl::GlslCompiler::makeShaderModuleCreateInfo(m_shaders.computeUpdateSceneRaster);

    vkCreateComputePipelines(res.m_device, nullptr, 1, &compInfo, nullptr, &m_pipelines.computeUpdateSceneRaster);
  }

  return true;
}


// 鍑芥暟锛歋ceneStreaming::deinitShadersAndPipelines銆傞噴鏀炬垨鍥炴敹鍓嶉潰鍒濆鍖栫殑璧勬簮锛屼繚鎸佺敓鍛藉懆鏈熸垚瀵圭鐞嗐€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氶噴鏀鹃『搴忚閬靛畧璧勬簮渚濊禆鍏崇郴锛岄伩鍏?GPU 浠嶅彲鑳借闂殑瀵硅薄琚彁鍓嶉攢姣併€?void SceneStreaming::deinitShadersAndPipelines()
void SceneStreaming::deinitShadersAndPipelines()
{
  Resources& res = *m_resources;


  res.destroyPipelines(m_pipelines);
}


}
