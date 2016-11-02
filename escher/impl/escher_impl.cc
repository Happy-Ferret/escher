// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "escher/impl/escher_impl.h"

#include "escher/impl/gpu_allocator.h"
#include "escher/impl/image_cache.h"
#include "escher/impl/mesh_manager.h"
#include "escher/impl/render_pass_manager.h"
#include "escher/util/cplusplus.h"

namespace escher {
namespace impl {

EscherImpl::EscherImpl(const VulkanContext& context,
                       const VulkanSwapchain& swapchain)
    : vulkan_context_(context),
      render_pass_manager_(std::make_unique<RenderPassManager>(context)),
      gpu_allocator_(std::make_unique<GpuAllocator>(context)),
      image_cache_(std::make_unique<ImageCache>(context.device,
                                                context.physical_device,
                                                gpu_allocator())),
      mesh_manager_(std::make_unique<MeshManager>(context, gpu_allocator())),
      renderer_count_(0) {
  FTL_DCHECK(context.instance);
  FTL_DCHECK(context.physical_device);
  FTL_DCHECK(context.device);
  FTL_DCHECK(context.queue);
  // TODO: additional validation, e.g. ensure that queue supports both graphics
  // and compute.
}

EscherImpl::~EscherImpl() {
  FTL_DCHECK(renderer_count_ == 0);

  vulkan_context_.device.waitIdle();
  mesh_manager_.reset();
  gpu_allocator_.reset();
}

ImageCache* EscherImpl::image_cache() {
  return image_cache_.get();
}

RenderPassManager* EscherImpl::render_pass_manager() {
  return render_pass_manager_.get();
}

MeshManager* EscherImpl::mesh_manager() {
  return mesh_manager_.get();
}

GpuAllocator* EscherImpl::gpu_allocator() {
  return gpu_allocator_.get();
}

const VulkanContext& EscherImpl::vulkan_context() {
  return vulkan_context_;
}

}  // namespace impl
}  // namespace escher
