// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "escher/escher.h"
#include "escher/impl/command_buffer_pool.h"
#include "escher/impl/escher_impl.h"
#include "escher/impl/glsl_compiler.h"
#include "escher/impl/image_cache.h"
#include "escher/impl/mesh_manager.h"
#include "escher/renderer/paper_renderer.h"
#include "escher/renderer/texture.h"
#include "escher/resources/resource_recycler.h"
#include "escher/util/image_utils.h"
#include "escher/vk/gpu_allocator.h"
#include "escher/vk/naive_gpu_allocator.h"

namespace escher {

namespace {

// Constructor helper.
std::unique_ptr<impl::CommandBufferPool> NewCommandBufferPool(
    const VulkanContext& context,
    impl::CommandBufferSequencer* sequencer) {
  return std::make_unique<impl::CommandBufferPool>(
      context.device, context.queue, context.queue_family_index, sequencer,
      true);
}

// Constructor helper.
std::unique_ptr<impl::CommandBufferPool> NewTransferCommandBufferPool(
    const VulkanContext& context,
    impl::CommandBufferSequencer* sequencer) {
  if (!context.transfer_queue)
    return nullptr;
  else
    return std::make_unique<impl::CommandBufferPool>(
        context.device, context.transfer_queue,
        context.transfer_queue_family_index, sequencer, false);
}

// Constructor helper.
std::unique_ptr<impl::GpuUploader> NewGpuUploader(
    Escher* escher,
    impl::CommandBufferPool* main_pool,
    impl::CommandBufferPool* transfer_pool,
    GpuAllocator* allocator) {
  return std::make_unique<impl::GpuUploader>(
      escher, transfer_pool ? transfer_pool : main_pool, allocator);
}

}  // anonymous namespace

Escher::Escher(VulkanDeviceQueuesPtr device)
    : device_(std::move(device)),
      vulkan_context_(device_->GetVulkanContext()),
      gpu_allocator_(std::make_unique<NaiveGpuAllocator>(vulkan_context_)),
      command_buffer_sequencer_(
          std::make_unique<impl::CommandBufferSequencer>()),
      command_buffer_pool_(
          NewCommandBufferPool(vulkan_context_,
                               command_buffer_sequencer_.get())),
      transfer_command_buffer_pool_(
          NewTransferCommandBufferPool(vulkan_context_,
                                       command_buffer_sequencer_.get())),
      glsl_compiler_(std::make_unique<impl::GlslToSpirvCompiler>()),
      image_cache_(std::make_unique<impl::ImageCache>(this, gpu_allocator())),
      gpu_uploader_(NewGpuUploader(this,
                                   command_buffer_pool(),
                                   transfer_command_buffer_pool(),
                                   gpu_allocator())),
      resource_recycler_(std::make_unique<ResourceRecycler>(this)),
      impl_(std::make_unique<impl::EscherImpl>(this, vulkan_context_)) {}

Escher::~Escher() {}

MeshBuilderPtr Escher::NewMeshBuilder(const MeshSpec& spec,
                                      size_t max_vertex_count,
                                      size_t max_index_count) {
  return impl_->mesh_manager()->NewMeshBuilder(spec, max_vertex_count,
                                               max_index_count);
}

ImagePtr Escher::NewRgbaImage(uint32_t width, uint32_t height, uint8_t* bytes) {
  return image_utils::NewRgbaImage(image_cache(), gpu_uploader(), width, height,
                                   bytes);
}

ImagePtr Escher::NewCheckerboardImage(uint32_t width, uint32_t height) {
  return image_utils::NewCheckerboardImage(image_cache(), gpu_uploader(), width,
                                           height);
}

ImagePtr Escher::NewGradientImage(uint32_t width, uint32_t height) {
  return image_utils::NewGradientImage(image_cache(), gpu_uploader(), width,
                                       height);
}

ImagePtr Escher::NewNoiseImage(uint32_t width, uint32_t height) {
  return image_utils::NewNoiseImage(image_cache(), gpu_uploader(), width,
                                    height);
}

PaperRendererPtr Escher::NewPaperRenderer() {
  return ftl::MakeRefCounted<PaperRenderer>(this);
}

TexturePtr Escher::NewTexture(ImagePtr image,
                              vk::Filter filter,
                              vk::ImageAspectFlags aspect_mask,
                              bool use_unnormalized_coordinates) {
  return ftl::MakeRefCounted<Texture>(resource_recycler(), std::move(image),
                                      filter, aspect_mask,
                                      use_unnormalized_coordinates);
}

uint64_t Escher::GetNumGpuBytesAllocated() {
  return gpu_allocator()->total_slab_bytes();
}

}  // namespace escher
