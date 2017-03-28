// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "escher/impl/escher_impl.h"

#include "escher/impl/command_buffer_pool.h"
#include "escher/impl/glsl_compiler.h"
#include "escher/impl/gpu_allocator.h"
#include "escher/impl/gpu_uploader.h"
#include "escher/impl/image_cache.h"
#include "escher/impl/mesh_manager.h"
#include "escher/impl/naive_gpu_allocator.h"
#include "escher/impl/vk/pipeline_cache.h"
#include "escher/profiling/timestamp_profiler.h"
#include "escher/resources/resource_life_preserver.h"
#include "escher/util/cplusplus.h"

namespace escher {
namespace impl {

namespace {

// Constructor helper.
std::unique_ptr<CommandBufferPool> NewCommandBufferPool(
    const VulkanContext& context,
    CommandBufferSequencer* sequencer) {
  return std::make_unique<CommandBufferPool>(context.device, context.queue,
                                             context.queue_family_index,
                                             sequencer, true);
}

// Constructor helper.
std::unique_ptr<CommandBufferPool> NewTransferCommandBufferPool(
    const VulkanContext& context,
    CommandBufferSequencer* sequencer) {
  if (!context.transfer_queue)
    return nullptr;
  else
    return std::make_unique<CommandBufferPool>(
        context.device, context.transfer_queue,
        context.transfer_queue_family_index, sequencer, false);
}

// Constructor helper.
std::unique_ptr<GpuUploader> NewGpuUploader(CommandBufferPool* main_pool,
                                            CommandBufferPool* transfer_pool,
                                            GpuAllocator* allocator) {
  return std::make_unique<GpuUploader>(
      transfer_pool ? transfer_pool : main_pool, allocator);
}

// Constructor helper.
std::unique_ptr<MeshManager> NewMeshManager(CommandBufferPool* main_pool,
                                            CommandBufferPool* transfer_pool,
                                            GpuAllocator* allocator,
                                            GpuUploader* uploader) {
  return std::make_unique<MeshManager>(
      transfer_pool ? transfer_pool : main_pool, allocator, uploader);
}

}  // namespace

EscherImpl::EscherImpl(const VulkanContext& context)
    : vulkan_context_(context),
      command_buffer_sequencer_(std::make_unique<CommandBufferSequencer>()),
      command_buffer_pool_(
          NewCommandBufferPool(context, command_buffer_sequencer_.get())),
      transfer_command_buffer_pool_(
          NewTransferCommandBufferPool(context,
                                       command_buffer_sequencer_.get())),
      gpu_allocator_(std::make_unique<NaiveGpuAllocator>(context)),
      gpu_uploader_(NewGpuUploader(command_buffer_pool(),
                                   transfer_command_buffer_pool(),
                                   gpu_allocator())),
      pipeline_cache_(std::make_unique<PipelineCache>()),
      image_cache_(std::make_unique<ImageCache>(vulkan_context_,
                                                command_buffer_pool(),
                                                gpu_allocator(),
                                                gpu_uploader())),
      mesh_manager_(NewMeshManager(command_buffer_pool(),
                                   transfer_command_buffer_pool(),
                                   gpu_allocator(),
                                   gpu_uploader())),
      glsl_compiler_(std::make_unique<GlslToSpirvCompiler>()),
      resource_life_preserver_(
          std::make_unique<ResourceLifePreserver>(vulkan_context_)),
      renderer_count_(0) {
  FTL_DCHECK(context.instance);
  FTL_DCHECK(context.physical_device);
  FTL_DCHECK(context.device);
  FTL_DCHECK(context.queue);
  // TODO: additional validation, e.g. ensure that queue supports both graphics
  // and compute.

  command_buffer_sequencer_->AddListener(resource_life_preserver_.get());

  auto device_properties = context.physical_device.getProperties();
  timestamp_period_ = device_properties.limits.timestampPeriod;
  auto queue_properties =
      context.physical_device
          .getQueueFamilyProperties()[context.queue_family_index];
  supports_timer_queries_ = queue_properties.timestampValidBits > 0;
}

EscherImpl::~EscherImpl() {
  FTL_DCHECK(renderer_count_ == 0);

  vulkan_context_.device.waitIdle();

  Cleanup();
}

void EscherImpl::Cleanup() {
  command_buffer_pool_->Cleanup();
  if (transfer_command_buffer_pool_)
    transfer_command_buffer_pool_->Cleanup();
}

const VulkanContext& EscherImpl::vulkan_context() {
  return vulkan_context_;
}

CommandBufferPool* EscherImpl::command_buffer_pool() {
  return command_buffer_pool_.get();
}

CommandBufferPool* EscherImpl::transfer_command_buffer_pool() {
  return transfer_command_buffer_pool_.get();
}

ImageCache* EscherImpl::image_cache() {
  return image_cache_.get();
}

MeshManager* EscherImpl::mesh_manager() {
  return mesh_manager_.get();
}

GlslToSpirvCompiler* EscherImpl::glsl_compiler() {
  return glsl_compiler_.get();
}

ResourceLifePreserver* EscherImpl::resource_life_preserver() {
  return resource_life_preserver_.get();
}

GpuAllocator* EscherImpl::gpu_allocator() {
  return gpu_allocator_.get();
}

GpuUploader* EscherImpl::gpu_uploader() {
  return gpu_uploader_.get();
}

}  // namespace impl
}  // namespace escher
