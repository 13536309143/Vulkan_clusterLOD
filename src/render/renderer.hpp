#pragma once
#if __INTELLISENSE__
#undef VK_NO_PROTOTYPES
#endif
#include <memory>

#include <nvvk/compute_pipeline.hpp>
#include "gpu/resources.hpp"
#include "scene/scene.hpp"
#include "render/preloaded_scene.hpp"
namespace lodclusters {

class RenderScene
{
public:
  const Scene*   scene        = nullptr;
  ScenePreloaded scenePreloaded;

  // pointers must stay valid during lifetime
  bool init(Resources* res, const Scene* scene_);
  void deinit();

  const nvvk::BufferTyped<shaderio::Geometry>& getShaderGeometriesBuffer() const;

  size_t                                       getOperationsSize() const;
  size_t                                       getGeometrySize() const;
};

struct RendererConfig
{
  bool flipWinding               = false;
  bool forceTwoSided             = false;
  bool useRenderStats            = false;
  bool useShading                = true;
  bool useDebugVisualization     = true;
  bool useEXTmeshShader          = false;
  bool useDepthOnly              = false;
  // the maximum number of renderable clusters per frame in bits i.e. (1 << number)
  uint32_t numRenderClusterBits = 22;
  // the maximum number of traversal intermediate tasks
  uint32_t numTraversalTaskBits = 22;
};

class Renderer
{
public:
  virtual bool init(Resources& res, RenderScene& rscene, const RendererConfig& config) = 0;
  virtual void render(VkCommandBuffer primary, Resources& res, RenderScene& rscene, const FrameConfig& frame, nvvk::ProfilerGpuTimer& profiler) = 0;
  virtual void deinit(Resources& res) = 0;
  virtual ~Renderer() {};  // Defined only so that inherited classes also have virtual destructors. Use deinit().
  virtual void updatedFrameBuffer(Resources& res, RenderScene& rscene) { updateBasicDescriptors(res, rscene); };
  struct ResourceUsageInfo
  {
    size_t operationsMemBytes{};
    size_t geometryMemBytes{};
    void add(const ResourceUsageInfo& other)
    {
      operationsMemBytes += other.operationsMemBytes;
      geometryMemBytes += other.geometryMemBytes;
    }
    size_t getTotalSum() const
    {
      return geometryMemBytes + operationsMemBytes;
    }
  };
  inline ResourceUsageInfo getResourceUsage(bool reserved) const
  {
    return reserved ? m_resourceReservedUsage : m_resourceActualUsage;
  };

  uint32_t getMaxRenderClusters() const { return m_maxRenderClusters; }
  uint32_t getMaxTraversalTasks() const { return m_maxTraversalTasks; }

protected:
  void initBasics(Resources& res, RenderScene& rscene, const RendererConfig& config);
  void deinitBasics(Resources& res);
  bool initBasicShaders(Resources& res, RenderScene& rscene, const RendererConfig& config);
  void initBasicPipelines(Resources& res, RenderScene& rscene, const RendererConfig& config);
  void updateBasicDescriptors(Resources& res, RenderScene& scene, const nvvk::Buffer* sceneBuildBuffer = nullptr);
  void writeBackgroundSky(VkCommandBuffer cmd);
  struct BasicShaders
  {
    shaderc::SpvCompilationResult fullScreenVertexShader;
    shaderc::SpvCompilationResult fullScreenBackgroundFragShader;
  };
  struct BasicPipelines
  {
    VkPipeline background{};
  };

  RendererConfig m_config;
  uint32_t       m_maxRenderClusters       = 0;
  uint32_t       m_maxTraversalTasks       = 0;
  uint32_t       m_meshShaderWorkgroupSize = 0;
  uint32_t       m_frameIndex              = 0;
  BasicShaders   m_basicShaders;
  BasicPipelines m_basicPipelines;

  std::vector<shaderio::RenderInstance> m_renderInstances;
  nvvk::Buffer                          m_renderInstanceBuffer;

  ResourceUsageInfo m_resourceReservedUsage{};
  ResourceUsageInfo m_resourceActualUsage{};

  nvvk::DescriptorPack m_basicDset;
  VkShaderStageFlags   m_basicShaderFlags{};
  VkPipelineLayout     m_basicPipelineLayout{};
};

/////////////////////////////////////////////////////////////////////////
inline float clusterLodErrorOverDistance(float errorSizeInPixels, float fov, float resolution)
{
  return (tanf(fov * 0.5f) * errorSizeInPixels / resolution);
}
//////////////////////////////////////////////////////////////////////////
std::unique_ptr<Renderer> makeRendererRasterClustersLod();

}
