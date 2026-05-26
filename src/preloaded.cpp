//直接加载
#include <limits>

#include <volk.h>
#include "preloaded.hpp"

namespace lodclusters {

namespace {

bool addSize(VkDeviceSize& total, VkDeviceSize value)
{
  if(value > std::numeric_limits<VkDeviceSize>::max() - total)
  {
    return false;
  }

  total += value;
  return true;
}

bool estimatePreloadSize(VkDeviceSize& estimatedSize, const Scene* scene)
{
  estimatedSize = 0;

  for(size_t geometryIndex = 0; geometryIndex < scene->getActiveGeometryCount(); geometryIndex++)
  {
    const Scene::GeometryView& sceneGeometry = scene->getActiveGeometry(geometryIndex);
    VkDeviceSize groupDataSize = sceneGeometry.groupData.size_bytes();
    if(scene->m_config.useCompressedData)
    {
      groupDataSize = 0;
      for(size_t g = 0; g < sceneGeometry.groupInfos.size(); g++)
      {
        if(!addSize(groupDataSize, sceneGeometry.groupInfos[g].getDeviceSize()))
          return false;
      }
    }

    const uint32_t numLodLevels = sceneGeometry.lodLevelsCount;
    const size_t   numNodes     = sceneGeometry.lodNodes.size();

    if(!addSize(estimatedSize, groupDataSize))
      return false;
    if(!addSize(estimatedSize, nvvk::BufferTyped<uint64_t>::value_size * sceneGeometry.groupInfos.size()))
      return false;
    if(!addSize(estimatedSize, nvvk::BufferTyped<uint64_t>::value_size * sceneGeometry.totalClustersCount))
      return false;
    if(!addSize(estimatedSize, nvvk::BufferTyped<shaderio::Node>::value_size * numNodes))
      return false;
    if(!addSize(estimatedSize, nvvk::BufferTyped<shaderio::BBox>::value_size * numNodes))
      return false;
    if(!addSize(estimatedSize, nvvk::BufferTyped<shaderio::LodLevel>::value_size * numLodLevels))
      return false;
  }

  return addSize(estimatedSize, nvvk::BufferTyped<shaderio::Geometry>::value_size * scene->getActiveGeometryCount());
}

}  // namespace

bool ScenePreloaded::canPreload(VkDeviceSize deviceLocalHeapSize, const Scene* scene)
{
  const VkDeviceSize sizeLimit = (deviceLocalHeapSize * 600) / 1000;
  VkDeviceSize       testSize  = 0;

  if(!estimatePreloadSize(testSize, scene) || testSize > sizeLimit)
  {
    LOGW("Preloaded scene too large: estimate %s, limit %s\n", formatMemorySize(testSize).c_str(),
         formatMemorySize(sizeLimit).c_str());
    return false;
  }

  LOGI("Preloaded scene memory estimate: %s / %s\n", formatMemorySize(testSize).c_str(), formatMemorySize(sizeLimit).c_str());
  return true;
}

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

  Resources::BatchedUploader uploader(*res);
  auto fail = [this](const char* what) {
    LOGE("Failed to allocate preloaded scene buffer: %s\n", what);
    deinit();
    return false;
  };

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

    if(res->createBuffer(preloadGeometry.groupData, groupDataSize,
                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
       != VK_SUCCESS)
    {
      return fail("groupData");
    }
    NVVK_DBG_NAME(preloadGeometry.groupData.buffer);

    if(res->createBufferTyped(preloadGeometry.groupAddresses, sceneGeometry.groupInfos.size(),
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
       != VK_SUCCESS)
    {
      return fail("groupAddresses");
    }
    if(res->createBufferTyped(preloadGeometry.clusterAddresses, sceneGeometry.totalClustersCount,
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
       != VK_SUCCESS)
    {
      return fail("clusterAddresses");
    }
    NVVK_DBG_NAME(preloadGeometry.groupAddresses.buffer);
    NVVK_DBG_NAME(preloadGeometry.clusterAddresses.buffer);

    size_t numNodes = sceneGeometry.lodNodes.size();
    if(res->createBufferTyped(preloadGeometry.lodNodes, numNodes,
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
       != VK_SUCCESS)
    {
      return fail("lodNodes");
    }
    if(res->createBufferTyped(preloadGeometry.lodNodeBboxes, numNodes,
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
       != VK_SUCCESS)
    {
      return fail("lodNodeBboxes");
    }
    NVVK_DBG_NAME(preloadGeometry.lodNodes.buffer);
    NVVK_DBG_NAME(preloadGeometry.lodNodeBboxes.buffer);

    uint32_t numLodLevels = sceneGeometry.lodLevelsCount;
    if(res->createBufferTyped(preloadGeometry.lodLevels, numLodLevels,
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
       != VK_SUCCESS)
    {
      return fail("lodLevels");
    }
    NVVK_DBG_NAME(preloadGeometry.lodLevels.buffer);

    m_geometrySize += preloadGeometry.groupData.bufferSize;
    m_geometrySize += preloadGeometry.groupAddresses.bufferSize;
    m_geometrySize += preloadGeometry.clusterAddresses.bufferSize;
    m_geometrySize += preloadGeometry.lodLevels.bufferSize;
    m_geometrySize += preloadGeometry.lodNodes.bufferSize;
    m_geometrySize += preloadGeometry.lodNodeBboxes.bufferSize;

    // setup shaderio
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

    // lowest detail group must have just a single cluster
    shaderio::LodLevel lastLodLevel = sceneGeometry.lodLevels.back();
    assert(lastLodLevel.groupCount == 1 && lastLodLevel.clusterCount == 1);

    shaderGeometry.lowDetailClusterID = lastLodLevel.clusterOffset;
    shaderGeometry.lowDetailTriangles = sceneGeometry.groupInfos[lastLodLevel.groupOffset].triangleCount;

    // basic uploads

    uploader.uploadBuffer(preloadGeometry.lodNodes, sceneGeometry.lodNodes.data());
    uploader.uploadBuffer(preloadGeometry.lodNodeBboxes, sceneGeometry.lodNodeBboxes.data());
    uploader.uploadBuffer(preloadGeometry.lodLevels, sceneGeometry.lodLevels.data());

    // clusters and groups need to be filled manually

    uint64_t* clusterAddresses = uploader.uploadBuffer(preloadGeometry.clusterAddresses, (uint64_t*)nullptr);
    uint64_t* groupAddresses =
        uploader.uploadBuffer(preloadGeometry.groupAddresses, (uint64_t*)nullptr, Resources::FlushState::DONT_FLUSH);
    uint8_t* groupData = uploader.uploadBuffer(preloadGeometry.groupData, (uint8_t*)nullptr, Resources::FlushState::DONT_FLUSH);

    uint32_t clusterOffset   = 0;
    size_t   groupDataOffset = 0;
    for(size_t g = 0; g < sceneGeometry.groupInfos.size(); g++)
    {
      const Scene::GroupInfo groupInfo = sceneGeometry.groupInfos[g];
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

  if(res->createBufferTyped(m_shaderGeometriesBuffer, scene->getActiveGeometryCount(),
                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
     != VK_SUCCESS)
  {
    return fail("shaderGeometries");
  }
  NVVK_DBG_NAME(m_shaderGeometriesBuffer.buffer);
  m_operationsSize += logMemoryUsage(m_shaderGeometriesBuffer.bufferSize, "operations", "preloaded geo buffer");

  uploader.uploadBuffer(m_shaderGeometriesBuffer, m_shaderGeometries.data());
  uploader.flush();

  return true;
}



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
  m_resources      = nullptr;
  m_scene          = nullptr;
  m_geometrySize   = 0;
  m_operationsSize = 0;
}
}  // namespace lodclusters
