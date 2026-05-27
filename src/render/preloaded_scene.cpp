#include <limits>
#include <algorithm>
#include <vector>

#include <volk.h>
#include "render/preloaded_scene.hpp"

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

VkDeviceSize alignUp(VkDeviceSize value, VkDeviceSize alignment)
{
  return (value + alignment - 1) & ~(alignment - 1);
}

VkDeviceSize getGeometryGroupDataSize(const Scene* scene, const Scene::GeometryView& sceneGeometry)
{
  VkDeviceSize groupDataSize = sceneGeometry.groupData.size_bytes();
  if(scene->m_config.useCompressedData)
  {
    groupDataSize = 0;
    for(size_t g = 0; g < sceneGeometry.groupInfos.size(); g++)
    {
      groupDataSize += sceneGeometry.groupInfos[g].getDeviceSize();
    }
  }

  return groupDataSize;
}

bool estimatePreloadSize(VkDeviceSize& estimatedSize, const Scene* scene)
{
  estimatedSize = 0;

  for(size_t geometryIndex = 0; geometryIndex < scene->getActiveGeometryCount(); geometryIndex++)
  {
    const Scene::GeometryView& sceneGeometry = scene->getActiveGeometry(geometryIndex);
    VkDeviceSize groupDataSize = getGeometryGroupDataSize(scene, sceneGeometry);

    const uint32_t numLodLevels = sceneGeometry.lodLevelsCount;
    const size_t   numNodes     = sceneGeometry.lodNodes.size();

    if(!addSize(estimatedSize, alignUp(groupDataSize, 16)))
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


  auto fail = [this](const char* what) {
    LOGE("Failed to allocate preloaded scene buffer: %s\n", what);
    deinit();
    return false;
  };

  const size_t geometryCount = scene->getActiveGeometryCount();
  m_shaderGeometries.resize(geometryCount);

  std::vector<VkDeviceSize> groupDataOffsets(geometryCount);
  std::vector<size_t>       groupAddressOffsets(geometryCount);
  std::vector<size_t>       clusterAddressOffsets(geometryCount);
  std::vector<size_t>       lodNodeOffsets(geometryCount);
  std::vector<size_t>       lodLevelOffsets(geometryCount);

  VkDeviceSize totalGroupDataSize    = 0;
  size_t       totalGroupAddresses   = 0;
  size_t       totalClusterAddresses = 0;
  size_t       totalLodNodes         = 0;
  size_t       totalLodLevels        = 0;

  for(size_t geometryIndex = 0; geometryIndex < geometryCount; geometryIndex++)
  {
    const Scene::GeometryView& sceneGeometry = scene->getActiveGeometry(geometryIndex);

    totalGroupDataSize              = alignUp(totalGroupDataSize, 16);
    groupDataOffsets[geometryIndex] = totalGroupDataSize;
    groupAddressOffsets[geometryIndex] = totalGroupAddresses;
    clusterAddressOffsets[geometryIndex] = totalClusterAddresses;
    lodNodeOffsets[geometryIndex] = totalLodNodes;
    lodLevelOffsets[geometryIndex] = totalLodLevels;

    totalGroupDataSize += getGeometryGroupDataSize(scene, sceneGeometry);
    totalGroupAddresses += sceneGeometry.groupInfos.size();
    totalClusterAddresses += sceneGeometry.totalClustersCount;
    totalLodNodes += sceneGeometry.lodNodes.size();
    totalLodLevels += sceneGeometry.lodLevelsCount;
  }

  totalGroupDataSize    = std::max<VkDeviceSize>(1, totalGroupDataSize);
  totalGroupAddresses   = std::max<size_t>(1, totalGroupAddresses);
  totalClusterAddresses = std::max<size_t>(1, totalClusterAddresses);
  totalLodNodes         = std::max<size_t>(1, totalLodNodes);
  totalLodLevels        = std::max<size_t>(1, totalLodLevels);

  if(res->createBuffer(m_geometryBuffers.groupData, totalGroupDataSize,
                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
     != VK_SUCCESS)
    return fail("groupData");
  if(res->createBufferTyped(m_geometryBuffers.groupAddresses, totalGroupAddresses,
                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
     != VK_SUCCESS)
    return fail("groupAddresses");
  if(res->createBufferTyped(m_geometryBuffers.clusterAddresses, totalClusterAddresses,
                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
     != VK_SUCCESS)
    return fail("clusterAddresses");
  if(res->createBufferTyped(m_geometryBuffers.lodNodes, totalLodNodes,
                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
     != VK_SUCCESS)
    return fail("lodNodes");
  if(res->createBufferTyped(m_geometryBuffers.lodNodeBboxes, totalLodNodes,
                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
     != VK_SUCCESS)
    return fail("lodNodeBboxes");
  if(res->createBufferTyped(m_geometryBuffers.lodLevels, totalLodLevels,
                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
     != VK_SUCCESS)
    return fail("lodLevels");

  NVVK_DBG_NAME(m_geometryBuffers.groupData.buffer);
  NVVK_DBG_NAME(m_geometryBuffers.groupAddresses.buffer);
  NVVK_DBG_NAME(m_geometryBuffers.clusterAddresses.buffer);
  NVVK_DBG_NAME(m_geometryBuffers.lodNodes.buffer);
  NVVK_DBG_NAME(m_geometryBuffers.lodNodeBboxes.buffer);
  NVVK_DBG_NAME(m_geometryBuffers.lodLevels.buffer);

  m_geometrySize += m_geometryBuffers.groupData.bufferSize;
  m_geometrySize += m_geometryBuffers.groupAddresses.bufferSize;
  m_geometrySize += m_geometryBuffers.clusterAddresses.bufferSize;
  m_geometrySize += m_geometryBuffers.lodLevels.bufferSize;
  m_geometrySize += m_geometryBuffers.lodNodes.bufferSize;
  m_geometrySize += m_geometryBuffers.lodNodeBboxes.bufferSize;

  LOGI("Preloaded scene uses global buffers for %zu geometries\n", geometryCount);

  Resources::BatchedUploader uploader(*res);

  uint32_t instancesOffset = 0;
  for(size_t geometryIndex = 0; geometryIndex < geometryCount; geometryIndex++)
  {
    shaderio::Geometry&        shaderGeometry  = m_shaderGeometries[geometryIndex];
    const Scene::GeometryView& sceneGeometry   = scene->getActiveGeometry(geometryIndex);

    uint32_t numLodLevels = sceneGeometry.lodLevelsCount;

    // setup shaderio
    shaderGeometry                    = {};
    shaderGeometry.bbox               = sceneGeometry.bbox;
    shaderGeometry.nodes              = m_geometryBuffers.lodNodes.address
                           + lodNodeOffsets[geometryIndex] * nvvk::BufferTyped<shaderio::Node>::value_size;
    shaderGeometry.nodeBboxes         = m_geometryBuffers.lodNodeBboxes.address
                           + lodNodeOffsets[geometryIndex] * nvvk::BufferTyped<shaderio::BBox>::value_size;
    shaderGeometry.preloadedGroups    = m_geometryBuffers.groupAddresses.address
                           + groupAddressOffsets[geometryIndex] * nvvk::BufferTyped<uint64_t>::value_size;
    shaderGeometry.preloadedClusters  = m_geometryBuffers.clusterAddresses.address
                           + clusterAddressOffsets[geometryIndex] * nvvk::BufferTyped<uint64_t>::value_size;
    shaderGeometry.lodLevelsCount     = uint32_t(numLodLevels);
    shaderGeometry.lodLevels          = m_geometryBuffers.lodLevels.address
                           + lodLevelOffsets[geometryIndex] * nvvk::BufferTyped<shaderio::LodLevel>::value_size;
    shaderGeometry.instancesCount     = sceneGeometry.instanceReferenceCount * scene->getGeometryInstanceFactor();
    shaderGeometry.instancesOffset    = instancesOffset;

    instancesOffset += shaderGeometry.instancesCount;

    // lowest detail group must have just a single cluster
    shaderio::LodLevel lastLodLevel = sceneGeometry.lodLevels.back();
    assert(lastLodLevel.groupCount == 1 && lastLodLevel.clusterCount == 1);

    shaderGeometry.lowDetailClusterID = lastLodLevel.clusterOffset;
    shaderGeometry.lowDetailTriangles = sceneGeometry.groupInfos[lastLodLevel.groupOffset].triangleCount;

    // basic uploads

    uploader.uploadBuffer(m_geometryBuffers.lodNodes,
                          lodNodeOffsets[geometryIndex] * nvvk::BufferTyped<shaderio::Node>::value_size,
                          sceneGeometry.lodNodes.size_bytes(), sceneGeometry.lodNodes.data());
    uploader.uploadBuffer(m_geometryBuffers.lodNodeBboxes,
                          lodNodeOffsets[geometryIndex] * nvvk::BufferTyped<shaderio::BBox>::value_size,
                          sceneGeometry.lodNodeBboxes.size_bytes(), sceneGeometry.lodNodeBboxes.data());
    uploader.uploadBuffer(m_geometryBuffers.lodLevels,
                          lodLevelOffsets[geometryIndex] * nvvk::BufferTyped<shaderio::LodLevel>::value_size,
                          sceneGeometry.lodLevels.size_bytes(), sceneGeometry.lodLevels.data());

    // clusters and groups need to be filled manually

    uint64_t* clusterAddresses = uploader.uploadBuffer(
        m_geometryBuffers.clusterAddresses,
        clusterAddressOffsets[geometryIndex] * nvvk::BufferTyped<uint64_t>::value_size,
        sceneGeometry.totalClustersCount * nvvk::BufferTyped<uint64_t>::value_size, (uint64_t*)nullptr);
    uint64_t* groupAddresses =
        uploader.uploadBuffer(m_geometryBuffers.groupAddresses,
                              groupAddressOffsets[geometryIndex] * nvvk::BufferTyped<uint64_t>::value_size,
                              sceneGeometry.groupInfos.size() * nvvk::BufferTyped<uint64_t>::value_size, (uint64_t*)nullptr,
                              Resources::FlushState::DONT_FLUSH);
    uint8_t* groupData =
        uploader.uploadBuffer(m_geometryBuffers.groupData, groupDataOffsets[geometryIndex],
                              getGeometryGroupDataSize(scene, sceneGeometry), (uint8_t*)nullptr, Resources::FlushState::DONT_FLUSH);

    uint32_t clusterOffset   = 0;
    size_t   groupDataOffset = 0;
    for(size_t g = 0; g < sceneGeometry.groupInfos.size(); g++)
    {
      const Scene::GroupInfo groupInfo = sceneGeometry.groupInfos[g];
      const Scene::GroupView groupView(sceneGeometry.groupData, groupInfo);
      uint64_t               groupVA = m_geometryBuffers.groupData.address + groupDataOffsets[geometryIndex] + groupDataOffset;

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

  if(res->createBufferTyped(m_shaderGeometriesBuffer, geometryCount,
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

  m_resources->m_allocator.destroyBuffer(m_geometryBuffers.clusterAddresses);
  m_resources->m_allocator.destroyBuffer(m_geometryBuffers.groupData);
  m_resources->m_allocator.destroyBuffer(m_geometryBuffers.groupAddresses);
  m_resources->m_allocator.destroyBuffer(m_geometryBuffers.lodNodes);
  m_resources->m_allocator.destroyBuffer(m_geometryBuffers.lodNodeBboxes);
  m_resources->m_allocator.destroyBuffer(m_geometryBuffers.lodLevels);
  m_geometryBuffers = {};

  m_resources->m_allocator.destroyBuffer(m_shaderGeometriesBuffer);
  m_resources      = nullptr;
  m_scene          = nullptr;
  m_geometrySize   = 0;
  m_operationsSize = 0;
}
}  // namespace lodclusters
