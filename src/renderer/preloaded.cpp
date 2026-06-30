//==============================================================================
// 鏂囦欢锛歴rc/renderer/preloaded.cpp
// 妯″潡瀹氫綅锛氶鍔犺浇 GPU 鍦烘櫙瀹炵幇锛屽垽鏂樉瀛樺閲忋€佸垱寤?缁?绨?鏁版嵁 缂撳啿 骞跺～鍏?鐫€鑹插櫒 鍦板潃銆?// 鏁版嵁娴侊細杈撳叆鏄?Scene 鍑犱綍瑙嗗浘锛涜緭鍑烘槸鍏ㄩ噺椹荤暀鐨?GPU 鏁版嵁鍜?Geometry 鍦板潃琛ㄣ€?// 鏂规硶璇存槑锛氳瀹炵幇鎶?CPU 鐨勫垎鏁?span 鎵撳寘鎴?GPU 杩炵画瀛樺偍锛屽噺灏戣繍琛屾椂鍦板潃淇ˉ鍜岀己椤靛鐞嗘垚鏈€?// 姝ｇ‘鎬х害鏉燂細瀹归噺浼扮畻瑕佷繚瀹堬紱姣忎釜 Geometry 鐨?low detail銆丩OD level銆乶ode 鍜?缁?鍦板潃蹇呴』瀵瑰簲姝ｇ‘ 缂撳啿 鍋忕Щ銆?// 娉ㄩ噴椋庢牸锛氫娇鐢ㄤ腑鏂囪В閲?CPU 渚ц涔夛紱淇濈暀蹇呰鐨?API銆佺被鍨嬪悕鍜屾暟瀛︾缉鍐欎互渚挎绱€?//==============================================================================
// 渚濊禆璇存槑锛氬紩鍏ユ湰缂栬瘧鍗曞厓闇€瑕佺殑澶栭儴搴撱€侀」鐩ā鍧楀拰鍏变韩鐫€鑹插櫒甯冨眬銆?// 渚濊禆椤哄簭閫氬父鍙嶆槧鎶借薄灞傛锛氬厛澶栭儴搴擄紝鍐嶉」鐩ā鍧楋紝鏈€鍚庝笌 GPU 鍏变韩鐨勬帴鍙ｅ畾涔夈€?#include <volk.h>
#include <volk.h>
#include "preloaded.hpp"


// 鍛藉悕绌洪棿璇存槑锛氶檺鍒剁鍙峰彲瑙佽寖鍥达紝骞惰〃鏄庤繖浜涚被鍨嬪拰鍑芥暟灞炰簬鍚屼竴鍔熻兘鍩熴€?// 璇ヨ竟鐣屾湁鍔╀簬鍖哄垎搴旂敤灞傘€佹覆鏌撳眰銆佸満鏅眰鍜岀畻娉曞眰鐨勮亴璐ｃ€?namespace lodclusters {
namespace lodclusters {


// 鍑芥暟锛歋cenePreloaded::canPreload銆備粠鏂囦欢銆佺紦瀛樸€丟PU 缂撳啿鎴栧叡浜竷灞€涓鍙栨暟鎹苟杞崲涓烘湰妯″潡鏍煎紡銆?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氳鍙栬矾寰勯渶瑕佹牎楠岃緭鍏ュ悎娉曟€э紝骞舵妸澶栭儴鏍煎紡鐨勪笉纭畾鎬ц浆鍖栦负鍐呴儴纭畾甯冨眬銆?bool ScenePreloaded::canPreload(VkDeviceSize deviceLocalHeapSize, const Scene* scene)
bool ScenePreloaded::canPreload(VkDeviceSize deviceLocalHeapSize, const Scene* scene)
{
  VkDeviceSize sizeLimit = (deviceLocalHeapSize * 600) / 1000;
  VkDeviceSize testSize  = 0;

  for(size_t geometryIndex = 0; geometryIndex < scene->getActiveGeometryCount(); geometryIndex++)
  {

    const Scene::GeometryView& sceneGeometry = scene->getActiveGeometry(geometryIndex);
    ScenePreloaded::Geometry   preloadGeometry;

    size_t numNodes = sceneGeometry.lodNodes.size();
    testSize += preloadGeometry.lodNodes.value_size * numNodes;
    testSize += preloadGeometry.lodNodeBboxes.value_size * numNodes;

    uint32_t numLodLevels = sceneGeometry.lodLevelsCount;
    testSize += preloadGeometry.lodLevels.value_size * numLodLevels;
  }

  if(testSize > sizeLimit)
  {

    LOGI("Likely exceeding device memory limit for preloaded scene\n");
    return false;
  }

  return true;
}


// 鍑芥暟锛歋cenePreloaded::init銆傚垵濮嬪寲鏈ā鍧楁墍闇€鐘舵€併€佽祫婧愭垨 GPU 渚х粦瀹氥€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氬垵濮嬪寲杩囩▼寤虹珛鍚庣画闃舵鍋囧畾瀛樺湪鐨勪笉鍙橀噺锛屼緥濡傚彞鏌勬湁鏁堛€佺紦鍐插ぇ灏忚冻澶熴€佹弿杩扮宸茬粦瀹氥€?bool ScenePreloaded::init(Resources* res, const Scene* scene, const Config& config)
bool ScenePreloaded::init(Resources* res, const Scene* scene, const Config& config)
{

  assert(m_resources == nullptr && "init called without prior deinit");

  m_resources = res;
  m_scene     = scene;
  m_config    = config;

  if(!canPreload(res->getDeviceLocalHeapSize(), scene))
  {

    LOGW("Likely exceeding device memory limit for preloaded scene\n");
    return false;
  }


  m_shaderGeometries.resize(scene->getActiveGeometryCount());
  m_geometries.resize(scene->getActiveGeometryCount());


  // 鍑芥暟锛歶ploader銆備粠鏂囦欢銆佺紦瀛樸€丟PU 缂撳啿鎴栧叡浜竷灞€涓鍙栨暟鎹苟杞崲涓烘湰妯″潡鏍煎紡銆?  // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?  // 璁捐瑕佺偣锛氳鍙栬矾寰勯渶瑕佹牎楠岃緭鍏ュ悎娉曟€э紝骞舵妸澶栭儴鏍煎紡鐨勪笉纭畾鎬ц浆鍖栦负鍐呴儴纭畾甯冨眬銆?  Resources::BatchedUploader uploader(*res);
  Resources::BatchedUploader uploader(*res);

  uint32_t instancesOffset = 0;
  for(size_t geometryIndex = 0; geometryIndex < scene->getActiveGeometryCount(); geometryIndex++)
  {
    shaderio::Geometry&        shaderGeometry  = m_shaderGeometries[geometryIndex];
    ScenePreloaded::Geometry&  preloadGeometry = m_geometries[geometryIndex];

    const Scene::GeometryView& sceneGeometry   = scene->getActiveGeometry(geometryIndex);


    size_t groupDataSize = sceneGeometry.groupData.size_bytes();

    if(scene->m_config.useCompressedData)
    {
      groupDataSize = 0;
      for(size_t g = 0; g < sceneGeometry.groupInfos.size(); g++)
      {
        const Scene::GroupInfo groupInfo = sceneGeometry.groupInfos[g];

        groupDataSize += groupInfo.getDeviceSize();
      }
    }

    res->createBuffer(preloadGeometry.groupData, groupDataSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    NVVK_DBG_NAME(preloadGeometry.groupData.buffer);

    res->createBufferTyped(preloadGeometry.groupAddresses, sceneGeometry.groupInfos.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    res->createBufferTyped(preloadGeometry.clusterAddresses, sceneGeometry.totalClustersCount, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    NVVK_DBG_NAME(preloadGeometry.groupAddresses.buffer);

    NVVK_DBG_NAME(preloadGeometry.clusterAddresses.buffer);


    size_t numNodes = sceneGeometry.lodNodes.size();

    res->createBufferTyped(preloadGeometry.lodNodes, numNodes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    res->createBufferTyped(preloadGeometry.lodNodeBboxes, numNodes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    NVVK_DBG_NAME(preloadGeometry.lodNodes.buffer);

    NVVK_DBG_NAME(preloadGeometry.lodNodeBboxes.buffer);

    uint32_t numLodLevels = sceneGeometry.lodLevelsCount;

    res->createBufferTyped(preloadGeometry.lodLevels, numLodLevels, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

    NVVK_DBG_NAME(preloadGeometry.lodLevels.buffer);

    m_geometrySize += preloadGeometry.groupData.bufferSize;
    m_geometrySize += preloadGeometry.groupAddresses.bufferSize;
    m_geometrySize += preloadGeometry.clusterAddresses.bufferSize;
    m_geometrySize += preloadGeometry.lodLevels.bufferSize;
    m_geometrySize += preloadGeometry.lodNodes.bufferSize;
    m_geometrySize += preloadGeometry.lodNodeBboxes.bufferSize;


    shaderGeometry                    = {};
    shaderGeometry.bbox               = sceneGeometry.bbox;
    shaderGeometry.nodes              = preloadGeometry.lodNodes.address;
    shaderGeometry.nodeBboxes         = preloadGeometry.lodNodeBboxes.address;
    shaderGeometry.preloadedGroups    = preloadGeometry.groupAddresses.address;
    shaderGeometry.preloadedClusters  = preloadGeometry.clusterAddresses.address;

    shaderGeometry.lodLevelsCount     = uint32_t(numLodLevels);
    shaderGeometry.lodLevels          = preloadGeometry.lodLevels.address;

    shaderGeometry.instancesCount     = sceneGeometry.instanceReferenceCount * scene->getGeometryInstanceFactor();
    shaderGeometry.instancesOffset    = instancesOffset;

    instancesOffset += shaderGeometry.instancesCount;


    shaderio::LodLevel lastLodLevel = sceneGeometry.lodLevels.back();

    assert(lastLodLevel.groupCount == 1 && lastLodLevel.clusterCount == 1);

    shaderGeometry.lowDetailClusterID = lastLodLevel.clusterOffset;
    shaderGeometry.lowDetailTriangles = sceneGeometry.groupInfos[lastLodLevel.groupOffset].triangleCount;


    uploader.uploadBuffer(preloadGeometry.lodNodes, sceneGeometry.lodNodes.data());
    uploader.uploadBuffer(preloadGeometry.lodNodeBboxes, sceneGeometry.lodNodeBboxes.data());
    uploader.uploadBuffer(preloadGeometry.lodLevels, sceneGeometry.lodLevels.data());


    uint64_t* clusterAddresses = uploader.uploadBuffer(preloadGeometry.clusterAddresses, (uint64_t*)nullptr);
    uint64_t* groupAddresses =
        uploader.uploadBuffer(preloadGeometry.groupAddresses, (uint64_t*)nullptr, Resources::FlushState::DONT_FLUSH);
    uint8_t* groupData = uploader.uploadBuffer(preloadGeometry.groupData, (uint8_t*)nullptr, Resources::FlushState::DONT_FLUSH);

    uint32_t clusterOffset   = 0;
    size_t   groupDataOffset = 0;
    for(size_t g = 0; g < sceneGeometry.groupInfos.size(); g++)
    {
      const Scene::GroupInfo groupInfo = sceneGeometry.groupInfos[g];


      // 鍑芥暟锛歡roupView銆傚皝瑁呮湰鏂囦欢涓殑涓€娈垫牳蹇冮€昏緫锛屼繚鎸佽皟鐢ㄦ柟鍙緷璧栨竻鏅扮殑鎺ュ彛璇箟銆?      // 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?      // 璁捐瑕佺偣锛氳鍑芥暟鐨勪富瑕佷环鍊煎湪浜庨殧绂诲眬閮ㄥ疄鐜扮粏鑺傦紝浣挎ā鍧楄竟鐣屽拰璋冪敤椤哄簭鏇村鏄撳鏌ャ€?      const Scene::GroupView groupView(sceneGeometry.groupData, groupInfo);
      const Scene::GroupView groupView(sceneGeometry.groupData, groupInfo);
      uint64_t               groupVA = preloadGeometry.groupData.address + groupDataOffset;

      groupAddresses[g] = groupVA;

      Scene::fillGroupRuntimeData(groupInfo, groupView, uint32_t(g), uint32_t(g), clusterOffset,
                                  groupData + groupDataOffset, groupInfo.getDeviceSize());


      groupDataOffset += groupInfo.getDeviceSize();

      for(uint32_t c = 0; c < groupInfo.clusterCount; c++)
      {
        clusterAddresses[c + clusterOffset] = groupVA + sizeof(shaderio::Group) + sizeof(shaderio::Cluster) * c;
      }

      clusterOffset += groupInfo.clusterCount;
    }
  }

  res->createBufferTyped(m_shaderGeometriesBuffer, scene->getActiveGeometryCount(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

  NVVK_DBG_NAME(m_shaderGeometriesBuffer.buffer);

  m_operationsSize += logMemoryUsage(m_shaderGeometriesBuffer.bufferSize, "operations", "preloaded geo buffer");

  uploader.uploadBuffer(m_shaderGeometriesBuffer, m_shaderGeometries.data());

  uploader.flush();

  return true;
}


// 鍑芥暟锛歋cenePreloaded::deinit銆傞噴鏀炬垨鍥炴敹鍓嶉潰鍒濆鍖栫殑璧勬簮锛屼繚鎸佺敓鍛藉懆鏈熸垚瀵圭鐞嗐€?// 杈撳叆/杈撳嚭锛氳緭鍏ョ敱鍙傛暟銆佹垚鍛樼姸鎬佹垨缁戝畾璧勬簮鎻愪緵锛涜緭鍑洪€氬父琛ㄧ幇涓鸿繑鍥炲€笺€佹垚鍛樼姸鎬佹洿鏂般€丟PU 缂撳啿鍐欏叆鎴栧懡浠ょ紦鍐茶褰曘€?// 璁捐瑕佺偣锛氶噴鏀鹃『搴忚閬靛畧璧勬簮渚濊禆鍏崇郴锛岄伩鍏?GPU 浠嶅彲鑳借闂殑瀵硅薄琚彁鍓嶉攢姣併€?void ScenePreloaded::deinit()
void ScenePreloaded::deinit()
{
  if(!m_resources)
    return;

  for(auto& it : m_geometries)
  {

    m_resources->m_allocator.destroyBuffer(it.clusterAddresses);

    m_resources->m_allocator.destroyBuffer(it.groupData);

    m_resources->m_allocator.destroyBuffer(it.groupAddresses);

    m_resources->m_allocator.destroyBuffer(it.lodNodes);

    m_resources->m_allocator.destroyBuffer(it.lodNodeBboxes);

    m_resources->m_allocator.destroyBuffer(it.lodLevels);
  }


  m_resources->m_allocator.destroyBuffer(m_shaderGeometriesBuffer);
  m_resources    = nullptr;
  m_scene        = nullptr;
  m_geometrySize = 0;
}
}
