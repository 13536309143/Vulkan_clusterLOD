#pragma once

#include <atomic>
#include <filesystem>
#include <memory>

#include <backends/imgui_impl_vulkan.h>
#include <nvapp/application.hpp>
#include <nvgui/enum_registry.hpp>
#include <nvvk/profiler_vk.hpp>
#include <nvutils/camera_manipulator.hpp>
#include <nvutils/parameter_parser.hpp>
#include <nvutils/parameter_sequencer.hpp>

#include "renderer.hpp"

namespace lodclusters {

class LodClusters : public nvapp::IAppElement
{
public:
  enum RendererType
  {
    RENDERER_RASTER_CLUSTERS_LOD,
  };

  enum ClusterConfig
  {
    CLUSTER_32T_32V,
    CLUSTER_64T_64V,
    CLUSTER_128T_128V,
    CLUSTER_256T_256V,
    NUM_CLUSTER_CONFIGS,
  };

  struct ClusterInfo
  {
    uint32_t      tris;
    uint32_t      verts;
    ClusterConfig cfg;
  };

  static const ClusterInfo s_clusterInfos[NUM_CLUSTER_CONFIGS];

  enum GuiEnums
  {
    GUI_SUPERSAMPLE,
    GUI_MESHLET,
    GUI_VISUALIZE,
  };

  struct Tweak
  {
    ClusterConfig clusterConfig = CLUSTER_128T_128V;
    int           supersample   = 2;
    bool          facetShading  = true;
  };

  struct Info
  {
    nvutils::ProfilerManager*                   profilerManager{};
    nvutils::ParameterRegistry*                 parameterRegistry{};
    nvutils::ParameterParser*                   parameterParser{};
    std::shared_ptr<nvutils::CameraManipulator> cameraManipulator;
  };

  explicit LodClusters(const Info& info);
  ~LodClusters() override;

  void onAttach(nvapp::Application* app) override;
  void onDetach() override;
  void onUIMenu() override;
  void onUIRender() override;
  void onPreRender() override {}
  void onRender(VkCommandBuffer cmd) override;
  void onResize(VkCommandBuffer cmd, const VkExtent2D& size) override;
  void onFileDrop(const std::filesystem::path& filename) override;

  void setSupportsBarycentrics(bool supported) { m_resources.m_supportsBarycentrics = supported; }
  void setSupportsMeshShaderNV(bool supported) { m_resources.m_supportsMeshShaderNV = supported; }
  void setSupportsSmBuiltinsNV(bool supported) { m_resources.m_supportsSmBuiltinsNV = supported; }
  bool getShowDebugUI() const { return false; }

  bool isProcessingOnly() const { return false; }
  void doProcessingOnly() {}
  void parameterSequenceCallback(const nvutils::ParameterSequencer::State& state);

private:
  void initScene(const std::filesystem::path& filePath, bool configChange);
  void deinitScene();
  void initRenderScene();
  void deinitRenderScene();
  void initRenderer();
  void deinitRenderer();
  void postInitNewScene();
  void setSceneCamera();
  void updateImguiImage();
  void handleChanges();

  void setFromClusterConfig(SceneConfig& sceneConfig, ClusterConfig clusterConfig);
  ClusterConfig findSceneClusterConfig(const SceneConfig& sceneConfig) const;

  VkExtent2D                 m_windowSize{};
  Info                       m_info;
  nvutils::ProfilerTimeline* m_profilerTimeline{};
  nvvk::ProfilerGpuTimer     m_profilerGpuTimer{};
  nvapp::Application*        m_app{};

  Resources           m_resources;
  FrameConfig         m_frameConfig;
  nvgui::EnumRegistry m_ui;
  VkDescriptorSet     m_imguiTexture{};
  VkSampler           m_imguiSampler{};
  nvutils::PerformanceTimer m_clock;

  Tweak          m_tweak;
  Tweak          m_tweakLast;
  SceneConfig    m_sceneConfig;
  SceneConfig    m_sceneConfigLast;
  RendererConfig m_rendererConfig;
  RendererConfig m_rendererConfigLast;

  std::unique_ptr<Scene>       m_scene;
  std::unique_ptr<RenderScene> m_renderScene;
  std::unique_ptr<Renderer>    m_renderer;
  uint64_t                     m_rendererFboChangeID{};

  std::filesystem::path m_sceneFilePath;
  std::filesystem::path m_sceneFilePathDropNew;
  std::filesystem::path m_sceneFilePathDropLast;
  std::atomic_bool      m_sceneLoading{false};
  std::atomic_uint32_t  m_sceneProgress{0};

  std::string m_cameraString;
  float       m_cameraSpeed = 0.0f;
  int         m_frames      = 0;
};

}  // namespace lodclusters
