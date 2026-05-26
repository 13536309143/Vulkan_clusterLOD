#include <algorithm>

#include <nvutils/file_operations.hpp>
#include <nvutils/logger.hpp>
#include <nvutils/spirv.hpp>
#include <nvvk/barriers.hpp>
#include <nvvk/formats.hpp>
#include <volk.h>

#include "resources.hpp"

namespace lodclusters {

void Resources::beginFrame(uint32_t cycleIndex)
{
  m_cycleIndex = cycleIndex;
}

void Resources::postProcessFrame(VkCommandBuffer cmd, const FrameConfig& frame, nvvk::ProfilerGpuTimer& profiler)
{
  (void)profiler;

  if(m_frameBuffer.useResolved)
  {
    VkImageBlit region{};
    region.dstOffsets[1]              = {int32_t(m_frameBuffer.windowSize.width), int32_t(m_frameBuffer.windowSize.height), 1};
    region.srcOffsets[1]              = {int32_t(m_frameBuffer.targetSize.width), int32_t(m_frameBuffer.targetSize.height), 1};
    region.dstSubresource.aspectMask  = VK_IMAGE_ASPECT_COLOR_BIT;
    region.dstSubresource.layerCount  = 1;
    region.srcSubresource.aspectMask  = VK_IMAGE_ASPECT_COLOR_BIT;
    region.srcSubresource.layerCount  = 1;

    cmdImageTransition(cmd, m_frameBuffer.imgColor, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    cmdImageTransition(cmd, m_frameBuffer.imgColorResolved, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkCmdBlitImage(cmd, m_frameBuffer.imgColor.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   m_frameBuffer.imgColorResolved.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_LINEAR);
    cmdImageTransition(cmd, m_frameBuffer.imgColorResolved, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }
  else
  {
    cmdImageTransition(cmd, m_frameBuffer.imgColor, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }

  // The simplified renderer does not use GPU picking or per-frame stats readback.
}

void Resources::endFrame() {}

void Resources::emptyFrame(VkCommandBuffer cmd, const FrameConfig&, nvvk::ProfilerGpuTimer& profiler)
{
  (void)profiler;
  cmdBeginRendering(cmd);
  vkCmdEndRendering(cmd);
}

void Resources::trackMemoryUsage(VkDeviceSize size, VmaMemoryUsage usage)
{
  m_memoryUsage.total += size;

  switch(usage)
  {
    case VMA_MEMORY_USAGE_GPU_ONLY:
      m_memoryUsage.deviceLocal += size;
      break;
    case VMA_MEMORY_USAGE_CPU_ONLY:
      m_memoryUsage.hostVisible += size;
      break;
    case VMA_MEMORY_USAGE_CPU_TO_GPU:
    case VMA_MEMORY_USAGE_GPU_TO_CPU:
      m_memoryUsage.hostCached += size;
      break;
    default:
      break;
  }
}

void Resources::logMemoryUsage() const
{
  LOGI("Memory Usage:\n");
  LOGI("  Total: %s\n", formatMemorySize(m_memoryUsage.total).c_str());
  LOGI("  Device Local: %s\n", formatMemorySize(m_memoryUsage.deviceLocal).c_str());
  LOGI("  Host Visible: %s\n", formatMemorySize(m_memoryUsage.hostVisible).c_str());
  LOGI("  Host Cached: %s\n", formatMemorySize(m_memoryUsage.hostCached).c_str());
}

void Resources::init(VkDevice device,
                     VkPhysicalDevice physicalDevice,
                     VkInstance instance,
                     const nvvk::QueueInfo& queue,
                     const nvvk::QueueInfo& queueTransfer)
{
  m_device         = device;
  m_physicalDevice = physicalDevice;
  m_queue          = queue;
  m_queueTransfer  = queueTransfer;

  m_physicalDeviceInfo.init(physicalDevice);
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &m_memoryProperties);
  m_use16bitDispatch = m_physicalDeviceInfo.properties10.limits.maxComputeWorkGroupCount[0] < (1 << 30);
  m_basicGraphicsState.depthStencilState.depthCompareOp = VK_COMPARE_OP_GREATER;

  VkPhysicalDeviceProperties2 props2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
  props2.pNext = &m_meshShaderPropsEXT;
  if(m_supportsMeshShaderNV)
  {
    m_meshShaderPropsNV.pNext = props2.pNext;
    props2.pNext              = &m_meshShaderPropsNV;
  }
  vkGetPhysicalDeviceProperties2(physicalDevice, &props2);

  VmaAllocatorCreateInfo allocatorInfo{
      .flags          = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
      .physicalDevice = physicalDevice,
      .device         = device,
      .instance       = instance,
  };
  NVVK_CHECK(m_allocator.init(allocatorInfo));

  m_uploader.init(&m_allocator);
  m_samplerPool.init(device);
  m_samplerPool.acquireSampler(m_samplerLinear);

  VkCommandPoolCreateInfo createInfo{
      .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = m_queue.familyIndex,
  };
  NVVK_CHECK(vkCreateCommandPool(m_device, &createInfo, nullptr, &m_tempCommandPool));

  m_cmdBufferCount = 32;
  m_cmdBuffers.resize(m_cmdBufferCount);
  m_cmdBuffersInUse.resize(m_cmdBufferCount, false);

  VkCommandBufferAllocateInfo allocInfo{
      .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool        = m_tempCommandPool,
      .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = m_cmdBufferCount,
  };
  NVVK_CHECK(vkAllocateCommandBuffers(m_device, &allocInfo, m_cmdBuffers.data()));
  m_tempCmdBufferPool = std::make_unique<MemoryPool>(4096, 32);

  std::filesystem::path exeDirectoryPath = nvutils::getExecutablePath().parent_path();
  const std::vector<std::filesystem::path> searchPaths = {
      std::filesystem::absolute(exeDirectoryPath / TARGET_EXE_TO_SOURCE_DIRECTORY / "shaders"),
      std::filesystem::absolute(exeDirectoryPath / TARGET_EXE_TO_NVSHADERS_DIRECTORY),
      std::filesystem::absolute(exeDirectoryPath / TARGET_NAME "_files" / "shaders"),
      std::filesystem::absolute(exeDirectoryPath),
  };
  m_glslCompiler.addSearchPaths(searchPaths);
  m_glslCompiler.defaultOptions();
  m_glslCompiler.defaultTarget();
  m_glslCompiler.options().SetGenerateDebugInfo();

  m_allocator.createBuffer(m_commonBuffers.frameConstants, sizeof(shaderio::FrameConstants),
                           VK_BUFFER_USAGE_2_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
                           VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
  m_allocator.createBuffer(m_commonBuffers.readBack, sizeof(shaderio::Readback),
                           VK_BUFFER_USAGE_2_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT
                               | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT | VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT,
                           VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
  m_allocator.createBuffer(m_commonBuffers.readBackHost, sizeof(shaderio::Readback) * 4,
                           VK_BUFFER_USAGE_2_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT,
                           VMA_MEMORY_USAGE_CPU_ONLY,
                           VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT);

  m_queueStates.primary.init(m_device, m_queue.queue, m_queue.familyIndex, 0);
  NVVK_DBG_NAME(m_queueStates.primary.m_timelineSemaphore);
  m_queueStates.transfer.init(m_device, m_queueTransfer.queue, m_queueTransfer.familyIndex, 0);
  NVVK_DBG_NAME(m_queueStates.transfer.m_timelineSemaphore);
}

void Resources::deinit()
{
  NVVK_CHECK(vkDeviceWaitIdle(m_device));

  m_allocator.destroyBuffer(m_commonBuffers.frameConstants);
  m_allocator.destroyBuffer(m_commonBuffers.readBack);
  m_allocator.destroyBuffer(m_commonBuffers.readBackHost);

  if(!m_cmdBuffers.empty())
  {
    vkFreeCommandBuffers(m_device, m_tempCommandPool, m_cmdBufferCount, m_cmdBuffers.data());
  }
  vkDestroyCommandPool(m_device, m_tempCommandPool, nullptr);

  deinitFramebuffer();
  m_queueStates.primary.deinit();
  m_queueStates.transfer.deinit();
  m_samplerPool.releaseSampler(m_samplerLinear);
  m_samplerPool.deinit();
  m_uploader.deinit();
  m_allocator.deinit();
}

bool Resources::initFramebuffer(const VkExtent2D& windowSize, int supersample)
{
  if(m_frameBuffer.imgColor.image != VK_NULL_HANDLE)
  {
    const bool sizeChanged = m_frameBuffer.windowSize.width != windowSize.width || m_frameBuffer.windowSize.height != windowSize.height;
    const bool supersampleChanged = m_frameBuffer.supersample != supersample;
    if(sizeChanged || supersampleChanged)
    {
      deinitFramebuffer();
    }
    else
    {
      return true;
    }
  }

  m_fboChangeID++;

  switch(supersample)
  {
    case 720:
      m_frameBuffer.targetSize = {1280, 720};
      break;
    case 1080:
      m_frameBuffer.targetSize = {1920, 1080};
      break;
    case 1440:
      m_frameBuffer.targetSize = {2560, 1440};
      break;
    case 2160:
      m_frameBuffer.targetSize = {3840, 2160};
      break;
    default:
      m_frameBuffer.targetSize.width  = windowSize.width * std::max(1, std::min(supersample, 4));
      m_frameBuffer.targetSize.height = windowSize.height * std::max(1, std::min(supersample, 4));
      break;
  }

  m_frameBuffer.renderSize  = m_frameBuffer.targetSize;
  m_frameBuffer.windowSize  = windowSize;
  m_frameBuffer.supersample = supersample;
  m_frameBuffer.useResolved = supersample > 1;
  m_frameBuffer.pixelScale  = std::max(float(m_frameBuffer.targetSize.width) / float(windowSize.width),
                                       float(m_frameBuffer.targetSize.height) / float(windowSize.height));
  m_basicGraphicsState.rasterizationState.lineWidth = m_frameBuffer.pixelScale;

  VkImageCreateInfo cbImageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  cbImageInfo.imageType     = VK_IMAGE_TYPE_2D;
  cbImageInfo.format        = m_frameBuffer.colorFormat;
  cbImageInfo.extent        = {m_frameBuffer.targetSize.width, m_frameBuffer.targetSize.height, 1};
  cbImageInfo.mipLevels     = 1;
  cbImageInfo.arrayLayers   = 1;
  cbImageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
  cbImageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
  cbImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  cbImageInfo.usage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                      | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

  VkImageViewCreateInfo cbImageViewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  cbImageViewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
  cbImageViewInfo.format                          = m_frameBuffer.colorFormat;
  cbImageViewInfo.subresourceRange.levelCount     = 1;
  cbImageViewInfo.subresourceRange.layerCount     = 1;
  cbImageViewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;

  NVVK_CHECK(m_allocator.createImage(m_frameBuffer.imgColor, cbImageInfo, cbImageViewInfo));
  NVVK_DBG_NAME(m_frameBuffer.imgColor.image);

  if(m_frameBuffer.useResolved)
  {
    VkImageCreateInfo resImageInfo = cbImageInfo;
    resImageInfo.extent            = {windowSize.width, windowSize.height, 1};
    NVVK_CHECK(m_allocator.createImage(m_frameBuffer.imgColorResolved, resImageInfo, cbImageViewInfo));
    NVVK_DBG_NAME(m_frameBuffer.imgColorResolved.image);
  }

  VkCommandBuffer cmd = createTempCmdBuffer();
  updateFramebufferRenderSizeDependent(cmd);
  tempSyncSubmit(cmd);

  VkPipelineRenderingCreateInfo pipelineRenderingInfo{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
  pipelineRenderingInfo.colorAttachmentCount    = 1;
  pipelineRenderingInfo.pColorAttachmentFormats = &m_frameBuffer.colorFormat;
  pipelineRenderingInfo.depthAttachmentFormat   = m_frameBuffer.depthStencilFormat;
  m_frameBuffer.pipelineRenderingInfo           = pipelineRenderingInfo;

  return true;
}

void Resources::updateFramebufferRenderSizeDependent(VkCommandBuffer)
{
  m_frameBuffer.depthStencilFormat = nvvk::findDepthStencilFormat(m_physicalDevice);

  VkImageCreateInfo dsImageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  dsImageInfo.imageType     = VK_IMAGE_TYPE_2D;
  dsImageInfo.format        = m_frameBuffer.depthStencilFormat;
  dsImageInfo.extent        = {m_frameBuffer.renderSize.width, m_frameBuffer.renderSize.height, 1};
  dsImageInfo.mipLevels     = 1;
  dsImageInfo.arrayLayers   = 1;
  dsImageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
  dsImageInfo.tiling        = VK_IMAGE_TILING_OPTIMAL;
  dsImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  dsImageInfo.usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

  VkImageViewCreateInfo dsImageViewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  dsImageViewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
  dsImageViewInfo.format                          = m_frameBuffer.depthStencilFormat;
  dsImageViewInfo.subresourceRange.levelCount     = 1;
  dsImageViewInfo.subresourceRange.layerCount     = 1;
  dsImageViewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT;

  NVVK_CHECK(m_allocator.createImage(m_frameBuffer.imgDepthStencil, dsImageInfo, dsImageViewInfo));
  NVVK_DBG_NAME(m_frameBuffer.imgDepthStencil.image);

  dsImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  dsImageViewInfo.image                       = m_frameBuffer.imgDepthStencil.image;
  NVVK_CHECK(vkCreateImageView(m_device, &dsImageViewInfo, nullptr, &m_frameBuffer.viewDepth));

  m_frameBuffer.viewport = {0.0f, 0.0f, float(m_frameBuffer.renderSize.width), float(m_frameBuffer.renderSize.height), 0.0f, 1.0f};
  m_frameBuffer.scissor  = {{0, 0}, m_frameBuffer.renderSize};
}

void Resources::deinitFramebufferRenderSizeDependent()
{
  m_allocator.destroyImage(m_frameBuffer.imgDepthStencil);
  vkDestroyImageView(m_device, m_frameBuffer.viewDepth, nullptr);
  m_frameBuffer.viewDepth = VK_NULL_HANDLE;
}

void Resources::deinitFramebuffer()
{
  NVVK_CHECK(vkDeviceWaitIdle(m_device));

  m_allocator.destroyImage(m_frameBuffer.imgColor);
  m_allocator.destroyImage(m_frameBuffer.imgColorResolved);
  deinitFramebufferRenderSizeDependent();
  m_frameBuffer = {};
}

glm::vec2 Resources::getFramebufferWindow2RenderScale() const
{
  if(m_frameBuffer.supersample >= 720)
  {
    return glm::vec2(1, 1);
  }

  return glm::vec2(m_frameBuffer.renderSize.width, m_frameBuffer.renderSize.height)
         / glm::vec2(m_frameBuffer.windowSize.width, m_frameBuffer.windowSize.height);
}

void Resources::getReadbackData(shaderio::Readback& readback)
{
  const shaderio::Readback* pReadback = m_commonBuffers.readBackHost.data();
  readback = pReadback[m_cycleIndex];
}

bool Resources::compileShader(shaderc::SpvCompilationResult& compiled,
                              VkShaderStageFlagBits shaderStage,
                              const std::filesystem::path& filePath,
                              shaderc::CompileOptions* options)
{
  compiled = m_glslCompiler.compileFile(filePath, nvvkglsl::getShaderKind(shaderStage), options);
  if(compiled.GetCompilationStatus() == shaderc_compilation_status_success)
  {
    if(m_dumpSpirv)
    {
      std::filesystem::path dumpFile = filePath.filename();
      dumpFile.replace_extension("spirv");
      nvutils::dumpSpirv(dumpFile, nvvkglsl::GlslCompiler::getSpirv(compiled),
                         nvvkglsl::GlslCompiler::getSpirvSize(compiled));
    }
    return true;
  }

  std::string errorMessage = compiled.GetErrorMessage();
  if(!errorMessage.empty())
  {
    nvutils::Logger::getInstance().log(nvutils::Logger::LogLevel::eWARNING, "%s", errorMessage.c_str());
  }
  return false;
}

VkCommandBuffer Resources::createTempCmdBuffer()
{
  for(uint32_t i = 0; i < m_cmdBufferCount; i++)
  {
    if(!m_cmdBuffersInUse[i])
    {
      m_cmdBuffersInUse[i] = true;
      VkCommandBuffer cmd  = m_cmdBuffers[i];
      VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
      beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
      NVVK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));
      return cmd;
    }
  }

  assert(false && "temporary command buffer pool exhausted");
  return VK_NULL_HANDLE;
}

void Resources::tempSyncSubmit(VkCommandBuffer cmd)
{
  vkEndCommandBuffer(cmd);

  VkCommandBufferSubmitInfo cmdInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
  cmdInfo.commandBuffer = cmd;
  VkSubmitInfo2 submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
  submitInfo.commandBufferInfoCount = 1;
  submitInfo.pCommandBufferInfos    = &cmdInfo;

  NVVK_CHECK(vkQueueSubmit2(m_queue.queue, 1, &submitInfo, nullptr));
  NVVK_CHECK(vkDeviceWaitIdle(m_device));

  for(uint32_t i = 0; i < m_cmdBufferCount; i++)
  {
    if(m_cmdBuffers[i] == cmd)
    {
      m_cmdBuffersInUse[i] = false;
      break;
    }
  }
}

void Resources::cmdBeginRendering(VkCommandBuffer cmd, bool hasSecondary, VkAttachmentLoadOp loadOpColor, VkAttachmentLoadOp loadOpDepth)
{
  VkClearValue colorClear{.color = {m_bgColor.x, m_bgColor.y, m_bgColor.z, m_bgColor.w}};
  VkClearValue depthClear{.depthStencil = {0.0F, 0}};

  cmdImageTransition(cmd, m_frameBuffer.imgColor, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  cmdImageTransition(cmd, m_frameBuffer.imgDepthStencil, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                     VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

  VkRenderingAttachmentInfo colorAttachment{
      .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView   = m_frameBuffer.imgColor.descriptor.imageView,
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp      = loadOpColor,
      .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue  = colorClear,
  };

  VkRenderingAttachmentInfo depthStencilAttachment{
      .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView   = m_frameBuffer.imgDepthStencil.descriptor.imageView,
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      .loadOp      = loadOpDepth,
      .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue  = depthClear,
  };

  VkRenderingInfo renderingInfo{
      .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .flags                = hasSecondary ? VK_RENDERING_CONTENTS_SECONDARY_COMMAND_BUFFERS_BIT : VkRenderingFlags(0),
      .renderArea           = m_frameBuffer.scissor,
      .layerCount           = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments    = &colorAttachment,
      .pDepthAttachment     = &depthStencilAttachment,
  };

  vkCmdBeginRendering(cmd, &renderingInfo);
  vkCmdSetViewportWithCount(cmd, 1, &m_frameBuffer.viewport);
  vkCmdSetScissorWithCount(cmd, 1, &m_frameBuffer.scissor);
}

void Resources::cmdImageTransition(VkCommandBuffer cmd, nvvk::Image& rimg, VkImageAspectFlags aspects, VkImageLayout newLayout,
                                   bool needBarrier) const
{
  if(rimg.image == VK_NULL_HANDLE || (newLayout == rimg.descriptor.imageLayout && !needBarrier))
  {
    return;
  }

  nvvk::ImageMemoryBarrierParams imageBarrier;
  imageBarrier.image                       = rimg.image;
  imageBarrier.oldLayout                   = rimg.descriptor.imageLayout;
  imageBarrier.newLayout                   = newLayout;
  imageBarrier.subresourceRange.aspectMask = aspects;
  nvvk::cmdImageMemoryBarrier(cmd, imageBarrier);
  rimg.descriptor.imageLayout = newLayout;
}

VkDeviceSize Resources::getDeviceLocalHeapSize() const
{
  for(uint32_t type = 0; type < m_memoryProperties.memoryTypeCount; type++)
  {
    if(m_memoryProperties.memoryTypes[type].propertyFlags == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    {
      return m_memoryProperties.memoryHeaps[m_memoryProperties.memoryTypes[type].heapIndex].size;
    }
  }

  for(uint32_t type = 0; type < m_memoryProperties.memoryTypeCount; type++)
  {
    const VkMemoryPropertyFlags flags = m_memoryProperties.memoryTypes[type].propertyFlags;
    if((flags & (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
       == (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
    {
      return m_memoryProperties.memoryHeaps[m_memoryProperties.memoryTypes[type].heapIndex].size;
    }
  }

  assert(false);
  return 0;
}

bool Resources::isBufferSizeValid(VkDeviceSize size) const
{
  return size <= m_physicalDeviceInfo.properties13.maxBufferSize && size <= m_physicalDeviceInfo.properties11.maxMemoryAllocationSize;
}

void QueueState::init(VkDevice device, VkQueue queue, uint32_t familyIndex, uint64_t initialValue)
{
  assert(m_device == nullptr);
  m_device      = device;
  m_queue       = queue;
  m_familyIndex = familyIndex;

  VkSemaphoreTypeCreateInfo timelineSemaphoreCreateInfo{
      .sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
      .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
      .initialValue  = initialValue,
  };
  VkSemaphoreCreateInfo semaphoreCreateInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
                                            .pNext = &timelineSemaphoreCreateInfo};
  vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &m_timelineSemaphore);
  m_timelineValue = initialValue + 1;
}

void QueueState::deinit()
{
  if(!m_device)
  {
    return;
  }
  vkDestroySemaphore(m_device, m_timelineSemaphore, nullptr);
}

VkSemaphoreSubmitInfo QueueState::getWaitSubmit(VkPipelineStageFlags2 stageMask, uint32_t deviceIndex) const
{
  return VkSemaphoreSubmitInfo{.sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                               .semaphore   = m_timelineSemaphore,
                               .value       = m_timelineValue,
                               .stageMask   = stageMask,
                               .deviceIndex = deviceIndex};
}

VkSemaphoreSubmitInfo QueueState::advanceSignalSubmit(VkPipelineStageFlags2 stageMask, uint32_t deviceIndex)
{
  return VkSemaphoreSubmitInfo{.sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                               .semaphore   = m_timelineSemaphore,
                               .value       = m_timelineValue++,
                               .stageMask   = stageMask,
                               .deviceIndex = deviceIndex};
}

}  // namespace lodclusters
