// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "escher/impl/command_buffer.h"

#include "escher/impl/descriptor_set_pool.h"
#include "escher/impl/mesh_shader_binding.h"
#include "escher/renderer/framebuffer.h"
#include "escher/renderer/image.h"
#include "escher/shape/mesh.h"
#include "escher/util/trace_macros.h"

#include "ftl/macros.h"

namespace escher {
namespace impl {

CommandBuffer::CommandBuffer(vk::Device device,
                             vk::CommandBuffer command_buffer,
                             vk::Fence fence,
                             vk::PipelineStageFlags pipeline_stage_mask)
    : device_(device),
      command_buffer_(command_buffer),
      fence_(fence),
      pipeline_stage_mask_(pipeline_stage_mask) {}

CommandBuffer::~CommandBuffer() {
  FTL_DCHECK(!is_active_ && !is_submitted_);
  // Owner is responsible for destroying command buffer and fence.
}

void CommandBuffer::Begin(uint64_t sequence_number) {
  FTL_DCHECK(!is_active_ && !is_submitted_);
  FTL_DCHECK(sequence_number > sequence_number_);
  is_active_ = true;
  sequence_number_ = sequence_number;
  auto result = command_buffer_.begin(vk::CommandBufferBeginInfo());
  FTL_DCHECK(result == vk::Result::eSuccess);
}

bool CommandBuffer::Submit(vk::Queue queue,
                           CommandBufferFinishedCallback callback) {
  TRACE_DURATION("gfx", "escher::CommandBuffer::Submit");

  FTL_DCHECK(is_active_ && !is_submitted_);
  is_submitted_ = true;
  callback_ = std::move(callback);

  auto end_command_buffer_result = command_buffer_.end();
  FTL_DCHECK(end_command_buffer_result == vk::Result::eSuccess);

  vk::SubmitInfo submit_info;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer_;
  submit_info.waitSemaphoreCount = wait_semaphores_for_submit_.size();
  submit_info.pWaitSemaphores = wait_semaphores_for_submit_.data();
  submit_info.pWaitDstStageMask = wait_semaphore_stages_.data();
  submit_info.signalSemaphoreCount = signal_semaphores_for_submit_.size();
  submit_info.pSignalSemaphores = signal_semaphores_for_submit_.data();

  auto submit_result = queue.submit(1, &submit_info, fence_);
  if (submit_result != vk::Result::eSuccess) {
    FTL_LOG(WARNING) << "failed queue submission: " << to_string(submit_result);
    // Clearing these flags allows Retire() to make progress.
    is_active_ = is_submitted_ = false;
    return false;
  }
  return true;
}

vk::Result CommandBuffer::Wait(uint64_t nanoseconds) {
  if (!is_active_) {
    // The command buffer is already finished.
    return vk::Result::eSuccess;
  }
  FTL_DCHECK(is_submitted_);
  return device_.waitForFences(1, &fence_, true, nanoseconds);
}

void CommandBuffer::AddWaitSemaphore(SemaphorePtr semaphore,
                                     vk::PipelineStageFlags stage) {
  FTL_DCHECK(is_active_);
  if (semaphore) {
    // Build up list that will be used when frame is submitted.
    wait_semaphores_for_submit_.push_back(semaphore->value());
    wait_semaphore_stages_.push_back(stage);
    // Retain semaphore to ensure that it doesn't prematurely die.
    wait_semaphores_.push_back(std::move(semaphore));
  }
}

void CommandBuffer::AddSignalSemaphore(SemaphorePtr semaphore) {
  FTL_DCHECK(is_active_);
  if (semaphore) {
    // Build up list that will be used when frame is submitted.
    signal_semaphores_for_submit_.push_back(semaphore->value());
    // Retain semaphore to ensure that it doesn't prematurely die.
    signal_semaphores_.push_back(std::move(semaphore));
  }
}

void CommandBuffer::KeepAlive(Resource* resource) {
  FTL_DCHECK(is_active_);
  if (sequence_number_ == resource->sequence_number()) {
    // The resource is already being kept alive by this CommandBuffer.
    return;
  }

  FTL_DCHECK(resource->sequence_number() <= sequence_number_);
  resource->set_sequence_number(sequence_number_);
  if (resource->IsKindOf<DescriptorSetAllocation>()) {
    // TODO: DescriptorSetPool will immediately recycle allocations, even while
    // they're still in use.  Therefore, we must ref the allocations until the
    // CommandBuffer has completed.  One way to fix this would be for
    // DescriptorSetPool to become a CommandBufferSequencerListener, similar to
    // ResourceRecycler.
    used_resources_.push_back(ResourcePtr(resource));
  }
}

void CommandBuffer::DrawMesh(const MeshPtr& mesh) {
  KeepAlive(mesh);

  AddWaitSemaphore(mesh->TakeWaitSemaphore(),
                   vk::PipelineStageFlagBits::eVertexInput);

  vk::Buffer vbo = mesh->vk_vertex_buffer();
  vk::DeviceSize vbo_offset = mesh->vertex_buffer_offset();
  uint32_t vbo_binding = MeshShaderBinding::kTheOnlyCurrentlySupportedBinding;
  command_buffer_.bindVertexBuffers(vbo_binding, 1, &vbo, &vbo_offset);
  command_buffer_.bindIndexBuffer(mesh->vk_index_buffer(),
                                  mesh->index_buffer_offset(),
                                  vk::IndexType::eUint32);
  command_buffer_.drawIndexed(mesh->num_indices(), 1, 0, 0, 0);
}

void CommandBuffer::CopyImage(const ImagePtr& src_image,
                              const ImagePtr& dst_image,
                              vk::ImageLayout src_layout,
                              vk::ImageLayout dst_layout,
                              vk::ImageCopy* region) {
  command_buffer_.copyImage(src_image->get(), src_layout, dst_image->get(),
                            dst_layout, 1, region);
  KeepAlive(src_image);
  KeepAlive(dst_image);
}

void CommandBuffer::TransitionImageLayout(const ImagePtr& image,
                                          vk::ImageLayout old_layout,
                                          vk::ImageLayout new_layout) {
  vk::PipelineStageFlags src_stage_mask;
  vk::PipelineStageFlags dst_stage_mask;

  vk::ImageMemoryBarrier barrier;
  barrier.oldLayout = old_layout;
  barrier.newLayout = new_layout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image->get();

  if (image->has_depth() || image->has_stencil()) {
    if (image->has_depth()) {
      barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
    }
    if (image->has_stencil()) {
      barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
    }
  } else {
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
  }

  // TODO: assert that image only has one level.
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  switch (old_layout) {
    case vk::ImageLayout::eColorAttachmentOptimal:
      barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead |
                              vk::AccessFlagBits::eColorAttachmentWrite;
      src_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
      break;
    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
      barrier.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead |
                              vk::AccessFlagBits::eDepthStencilAttachmentWrite;
      src_stage_mask = vk::PipelineStageFlagBits::eLateFragmentTests;
      break;
    case vk::ImageLayout::eGeneral:
      barrier.srcAccessMask =
          vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;
      src_stage_mask = vk::PipelineStageFlagBits::eComputeShader;
      break;
    case vk::ImageLayout::ePreinitialized:
      barrier.srcAccessMask = vk::AccessFlagBits::eHostWrite;
      src_stage_mask = vk::PipelineStageFlagBits::eTopOfPipe;
      break;
    case vk::ImageLayout::eShaderReadOnlyOptimal:
      barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
      // TODO: investigate whether there are performance benefits to providing
      // a less-conservative mask.
      src_stage_mask =
          vk::PipelineStageFlagBits::eVertexShader |
          vk::PipelineStageFlagBits::eTessellationControlShader |
          vk::PipelineStageFlagBits::eTessellationEvaluationShader |
          vk::PipelineStageFlagBits::eGeometryShader |
          vk::PipelineStageFlagBits::eFragmentShader;
      break;
    case vk::ImageLayout::eTransferDstOptimal:
      barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
      src_stage_mask = vk::PipelineStageFlagBits::eTransfer;
      break;
    case vk::ImageLayout::eUndefined:
      // If layout was eUndefined, we don't need a srcAccessMask.
      src_stage_mask = vk::PipelineStageFlagBits::eTopOfPipe;
      break;
    default:
      FTL_LOG(ERROR)
          << "CommandBuffer does not know how to transition from layout: "
          << vk::to_string(old_layout);
      FTL_DCHECK(false);
  }

  switch (new_layout) {
    case vk::ImageLayout::eColorAttachmentOptimal:
      barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead |
                              vk::AccessFlagBits::eColorAttachmentWrite;
      dst_stage_mask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
      break;
    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
      barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead |
                              vk::AccessFlagBits::eDepthStencilAttachmentWrite;
      dst_stage_mask = vk::PipelineStageFlagBits::eEarlyFragmentTests;
      break;
    case vk::ImageLayout::eGeneral:
      barrier.dstAccessMask =
          vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;
      dst_stage_mask = vk::PipelineStageFlagBits::eComputeShader;
      break;
    case vk::ImageLayout::ePresentSrcKHR:
      barrier.dstAccessMask = vk::AccessFlagBits::eMemoryRead;
      dst_stage_mask = vk::PipelineStageFlagBits::eTransfer;
      break;
    case vk::ImageLayout::eShaderReadOnlyOptimal:
      barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
      // TODO: investigate whether there are performance benefits to providing
      // a less-conservative mask.
      dst_stage_mask =
          vk::PipelineStageFlagBits::eVertexShader |
          vk::PipelineStageFlagBits::eTessellationControlShader |
          vk::PipelineStageFlagBits::eTessellationEvaluationShader |
          vk::PipelineStageFlagBits::eGeometryShader |
          vk::PipelineStageFlagBits::eFragmentShader;

      if ((dst_stage_mask & pipeline_stage_mask_) == vk::PipelineStageFlags()) {
        // We must be on a queue that doesn't support graphics operations.
        dst_stage_mask = vk::PipelineStageFlagBits::eTopOfPipe;
      }
      break;
    case vk::ImageLayout::eTransferDstOptimal:
      barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
      dst_stage_mask = vk::PipelineStageFlagBits::eTransfer;
      break;
    case vk::ImageLayout::eTransferSrcOptimal:
      barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;
      dst_stage_mask = vk::PipelineStageFlagBits::eTransfer;
      break;
    default:
      FTL_LOG(ERROR)
          << "CommandBuffer does not know how to transition to layout: "
          << vk::to_string(new_layout);
      FTL_DCHECK(false);
  }

  src_stage_mask = src_stage_mask & pipeline_stage_mask_;
  dst_stage_mask = dst_stage_mask & pipeline_stage_mask_;

  command_buffer_.pipelineBarrier(src_stage_mask, dst_stage_mask,
                                  vk::DependencyFlagBits::eByRegion, 0, nullptr,
                                  0, nullptr, 1, &barrier);

  KeepAlive(image);
}

void CommandBuffer::BeginRenderPass(
    vk::RenderPass render_pass,
    const FramebufferPtr& framebuffer,
    const std::vector<vk::ClearValue>& clear_values) {
  BeginRenderPass(render_pass, framebuffer, clear_values.data(),
                  clear_values.size());
}

void CommandBuffer::BeginRenderPass(vk::RenderPass render_pass,
                                    const FramebufferPtr& framebuffer,
                                    const vk::ClearValue* clear_values,
                                    size_t clear_value_count) {
  FTL_DCHECK(is_active_);
  uint32_t width = framebuffer->width();
  uint32_t height = framebuffer->height();

  vk::RenderPassBeginInfo info;
  info.renderPass = render_pass;
  info.renderArea.offset.x = 0;
  info.renderArea.offset.y = 0;
  info.renderArea.extent.width = width;
  info.renderArea.extent.height = height;
  info.clearValueCount = static_cast<uint32_t>(clear_value_count);
  info.pClearValues = clear_values;
  info.framebuffer = framebuffer->get();

  command_buffer_.beginRenderPass(&info, vk::SubpassContents::eInline);

  vk::Viewport viewport;
  viewport.width = static_cast<float>(width);
  viewport.height = static_cast<float>(height);
  viewport.minDepth = static_cast<float>(0.0f);
  viewport.maxDepth = static_cast<float>(1.0f);
  command_buffer_.setViewport(0, 1, &viewport);

  // TODO: probably unnecessary?
  vk::Rect2D scissor;
  scissor.extent.width = width;
  scissor.extent.height = height;
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  command_buffer_.setScissor(0, 1, &scissor);

  // TODO: should we retain the framebuffer?
}

bool CommandBuffer::Retire() {
  if (!is_active_) {
    // Submission failed, so proceed with cleanup.
    FTL_DLOG(INFO)
        << "CommandBuffer submission failed, proceeding with retirement";
  } else if (!is_submitted_) {
    return false;
  } else {
    FTL_DCHECK(is_active_);
    // Check if fence has been reached.
    auto fence_status = device_.getFenceStatus(fence_);
    if (fence_status == vk::Result::eNotReady) {
      // Fence has not been reached; try again later.
      return false;
    }
  }
  is_active_ = is_submitted_ = false;
  device_.resetFences(1, &fence_);

  used_resources_.clear();

  if (callback_) {
    callback_();
    callback_ = nullptr;
  }

  // TODO: move semaphores to pool for reuse?
  wait_semaphores_.clear();
  wait_semaphores_for_submit_.clear();
  wait_semaphore_stages_.clear();
  signal_semaphores_.clear();
  signal_semaphores_for_submit_.clear();

  auto result = command_buffer_.reset(vk::CommandBufferResetFlags());
  FTL_DCHECK(result == vk::Result::eSuccess);

  return true;
}

}  // namespace impl
}  // namespace escher
