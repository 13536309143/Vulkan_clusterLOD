#pragma once

#include "scene/scene.hpp"
#include "gpu/resources.hpp"

namespace lodclusters {

// Owns the fully resident GPU data for all cluster LOD levels.
class ScenePreloaded
{
public:
  struct Config
  {
  };

  static bool canPreload(VkDeviceSize, const Scene* scene);

  bool init(Resources* res, const Scene* scene, const Config& config);

  void deinit();

  const nvvk::BufferTyped<shaderio::Geometry>& getShaderGeometriesBuffer() const { return m_shaderGeometriesBuffer; }

  size_t getGeometrySize() const { return m_geometrySize; }
  size_t getOperationsSize() const { return m_operationsSize; }

private:
  struct GeometryBuffers
  {
    nvvk::BufferTyped<shaderio::LodLevel> lodLevels;
    nvvk::BufferTyped<shaderio::Node>     lodNodes;
    nvvk::BufferTyped<shaderio::BBox>     lodNodeBboxes;

    nvvk::Buffer                groupData;
    nvvk::BufferTyped<uint64_t> groupAddresses;
    nvvk::BufferTyped<uint64_t> clusterAddresses;
  };

  Config       m_config;
  Resources*   m_resources = nullptr;
  const Scene* m_scene     = nullptr;

  size_t m_geometrySize   = 0;
  size_t m_operationsSize = 0;

  GeometryBuffers                 m_geometryBuffers;
  std::vector<shaderio::Geometry> m_shaderGeometries;

  nvvk::BufferTyped<shaderio::Geometry> m_shaderGeometriesBuffer;
};
}  // namespace lodclusters
