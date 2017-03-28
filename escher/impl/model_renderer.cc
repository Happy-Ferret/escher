// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "escher/impl/model_renderer.h"

#include <glm/gtx/transform.hpp>
#include "escher/geometry/tessellation.h"
#include "escher/impl/command_buffer.h"
#include "escher/impl/escher_impl.h"
#include "escher/impl/image_cache.h"
#include "escher/impl/mesh_impl.h"
#include "escher/impl/mesh_manager.h"
#include "escher/impl/model_data.h"
#include "escher/impl/model_display_list.h"
#include "escher/impl/model_display_list_builder.h"
#include "escher/impl/model_pipeline.h"
#include "escher/impl/model_pipeline_cache.h"
#include "escher/impl/vulkan_utils.h"
#include "escher/renderer/image.h"
#include "escher/scene/model.h"
#include "escher/scene/shape.h"
#include "escher/scene/stage.h"
#include "escher/util/image_loader.h"

namespace escher {
namespace impl {

ModelRenderer::ModelRenderer(EscherImpl* escher,
                             ModelData* model_data,
                             vk::Format pre_pass_color_format,
                             vk::Format lighting_pass_color_format,
                             vk::Format depth_format)
    : device_(escher->vulkan_context().device),
      life_preserver(escher->resource_life_preserver()),
      mesh_manager_(escher->mesh_manager()),
      model_data_(model_data) {
  rectangle_ = CreateRectangle();
  circle_ = CreateCircle();
  white_texture_ = CreateWhiteTexture(escher);

  CreateRenderPasses(pre_pass_color_format, lighting_pass_color_format,
                     depth_format);
  pipeline_cache_ = std::make_unique<impl::ModelPipelineCacheOLD>(
      device_, depth_prepass_, lighting_pass_, model_data_, mesh_manager_);
}

ModelRenderer::~ModelRenderer() {
  device_.destroyRenderPass(depth_prepass_);
  device_.destroyRenderPass(lighting_pass_);
}

ModelDisplayListPtr ModelRenderer::CreateDisplayList(
    const Stage& stage,
    const Model& model,
    vec2 scale,
    bool sort_by_pipeline,
    bool use_depth_prepass,
    bool use_descriptor_set_per_object,
    uint32_t sample_count,
    const TexturePtr& illumination_texture,
    CommandBuffer* command_buffer) {
  const std::vector<Object>& objects = model.objects();

  // The alternative isn't implemented.
  FTL_DCHECK(use_descriptor_set_per_object);

  // Used to accumulate indices of objects in render-order.
  std::vector<uint32_t> opaque_objects;
  opaque_objects.reserve(objects.size());
  // TODO: Translucency.  When rendering translucent objects, we will need a
  // separate bin for all translucent objects, and need to sort the objects in
  // that bin from back-to-front.  Conceivably, we could relax this ordering
  // requirement in cases where we can prove that the translucent objects don't
  // overlap.

  // TODO: We should sort according to more different metrics, and look for
  // performance differences between them.  At the same time, we should
  // experiment with strategies for updating/binding descriptor-sets.
  if (!sort_by_pipeline) {
    // Simply render objects in the order that they appear in the model.
    for (uint32_t i = 0; i < objects.size(); ++i) {
      opaque_objects.push_back(i);
    }
  } else {
    // Sort all objects into bins.  Then, iterate over each bin in arbitrary
    // order, without additional sorting within the bin.
    std::unordered_map<ModelPipelineSpec, std::vector<size_t>,
                       Hash<ModelPipelineSpec>>
        pipeline_bins;
    for (size_t i = 0; i < objects.size(); ++i) {
      ModelPipelineSpec spec;
      auto& obj = objects[i];
      spec.mesh_spec = GetMeshForShape(obj.shape())->spec;
      spec.shape_modifiers = obj.shape().modifiers();
      pipeline_bins[spec].push_back(i);
    }
    for (auto& pair : pipeline_bins) {
      for (uint32_t object_index : pair.second) {
        opaque_objects.push_back(object_index);
      }
    }
  }
  FTL_DCHECK(opaque_objects.size() == objects.size());

  ModelDisplayListBuilder builder(
      device_, stage, model, scale, !use_depth_prepass, white_texture_,
      illumination_texture, model_data_, this, pipeline_cache_.get(),
      sample_count, use_depth_prepass);
  for (uint32_t object_index : opaque_objects) {
    builder.AddObject(objects[object_index]);
  }
  return builder.Build(command_buffer);
}

// TODO: stage shouldn't be necessary.
void ModelRenderer::Draw(const Stage& stage,
                         const ModelDisplayListPtr& display_list,
                         CommandBuffer* command_buffer) {
  vk::CommandBuffer vk_command_buffer = command_buffer->get();

  for (const TexturePtr& texture : display_list->textures()) {
    // TODO: it would be nice if Resource::TakeWaitSemaphore() were virtual
    // so that we could say texture->TakeWaitSemaphore(), instead of needing
    // to know that the image is really the thing that we might need to wait
    // for.  Another approach would be for the Texture constructor to say
    // SetWaitSemaphore(image->TakeWaitSemaphore()), but this isn't a
    // bulletproof solution... what if someone else made a Texture with the
    // same image, and used that one first.  Of course, in general we want
    // lighter-weight synchronization such as events or barriers... need to
    // revisit this whole topic.
    command_buffer->AddWaitSemaphore(
        texture->image()->TakeWaitSemaphore(),
        vk::PipelineStageFlagBits::eFragmentShader);
  }

  auto& volume = stage.viewing_volume();
  // We assume that we are looking down at the stage, so volume.near() equals
  // the maximum height above the stage.
  FTL_DCHECK(volume.far() == 0 && volume.near() > 0);

  vk::Viewport viewport;
  viewport.width = volume.width();
  viewport.height = volume.height();
  // We normalize all depths to the range [0,1].  If we didn't, then Vulkan
  // would clip them anyway.  NOTE: this is only true because we are using an
  // orthonormal projection; otherwise the depth computed by the vertex shader
  // could be outside [0,1] as long as the perspective division brought it back.
  // In this case, it might make sense to use different values for viewport
  // min/max depth.
  viewport.minDepth = 0.f;
  viewport.maxDepth = 1.f;
  vk_command_buffer.setViewport(0, 1, &viewport);

  // Retain all display-list resources until the frame is finished rendering.
  command_buffer->AddUsedResource(display_list);

  vk::Pipeline current_pipeline;
  vk::PipelineLayout current_pipeline_layout;
  for (const ModelDisplayList::Item& item : display_list->items()) {
    // Bind new pipeline and PerModel descriptor set, if necessary.
    if (current_pipeline != item.pipeline->pipeline()) {
      current_pipeline = item.pipeline->pipeline();
      vk_command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                                     current_pipeline);

      // Whenever the pipeline changes, it is possible that the pipeline layout
      // must also change.
      if (current_pipeline_layout != item.pipeline->pipeline_layout()) {
        current_pipeline_layout = item.pipeline->pipeline_layout();
        vk::DescriptorSet ds = display_list->stage_data();
        vk_command_buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, current_pipeline_layout,
            ModelData::PerModel::kDescriptorSetIndex, 1, &ds, 0, nullptr);
      }
    }

    vk::DescriptorSet ds = item.descriptor_sets[0];
    vk_command_buffer.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics, current_pipeline_layout,
        ModelData::PerObject::kDescriptorSetIndex, 1, &ds, 0, nullptr);

    command_buffer->DrawMesh(item.mesh);
  }
}

const MeshPtr& ModelRenderer::GetMeshForShape(const Shape& shape) const {
  switch (shape.type()) {
    case Shape::Type::kRect:
      return rectangle_;
    case Shape::Type::kCircle:
      return circle_;
    case Shape::Type::kMesh:
      return shape.mesh();
  }
  FTL_CHECK(false);
  return shape.mesh();  // this would DCHECK
}

MeshPtr ModelRenderer::CreateRectangle() {
  return NewSimpleRectangleMesh(mesh_manager_);
}

MeshPtr ModelRenderer::CreateCircle() {
  MeshSpec spec{MeshAttribute::kPosition | MeshAttribute::kUV};

  return NewCircleMesh(mesh_manager_, spec, 4, vec2(0.5f, 0.5f), 0.5f);
}

TexturePtr ModelRenderer::CreateWhiteTexture(EscherImpl* escher) {
  uint8_t channels[4];
  channels[0] = channels[1] = channels[2] = channels[3] = 255;

  auto image = escher->image_cache()->NewRgbaImage(1, 1, channels);
  return ftl::MakeRefCounted<Texture>(escher->resource_life_preserver(),
                                      std::move(image), vk::Filter::eNearest);
}

void ModelRenderer::CreateRenderPasses(vk::Format pre_pass_color_format,
                                       vk::Format lighting_pass_color_format,
                                       vk::Format depth_format) {
  constexpr uint32_t kAttachmentCount = 2;
  const uint32_t kColorAttachment = 0;
  const uint32_t kDepthAttachment = 1;
  vk::AttachmentDescription attachments[kAttachmentCount];
  auto& color_attachment = attachments[kColorAttachment];
  auto& depth_attachment = attachments[kDepthAttachment];

  // Load/store ops and image layouts differ between passes; see below.
  depth_attachment.format = depth_format;
  depth_attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
  depth_attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;

  vk::AttachmentReference color_reference;
  color_reference.attachment = kColorAttachment;
  color_reference.layout = vk::ImageLayout::eColorAttachmentOptimal;

  vk::AttachmentReference depth_reference;
  depth_reference.attachment = kDepthAttachment;
  depth_reference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

  // Every vk::RenderPass needs at least one subpass.
  vk::SubpassDescription subpass;
  subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_reference;
  subpass.pDepthStencilAttachment = &depth_reference;
  subpass.inputAttachmentCount = 0;  // no other subpasses to sample from

  // Even though we have a single subpass, we need to declare dependencies to
  // support the layout transitions specified by the attachment references.
  constexpr uint32_t kDependencyCount = 2;
  vk::SubpassDependency dependencies[kDependencyCount];
  auto& input_dependency = dependencies[0];
  auto& output_dependency = dependencies[1];

  // The first dependency transitions from the final layout from the previous
  // render pass, to the initial layout of this one.
  input_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;  // not in vulkan.hpp ?!?
  input_dependency.dstSubpass = 0;
  input_dependency.srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
  input_dependency.dstStageMask =
      vk::PipelineStageFlagBits::eColorAttachmentOutput;
  input_dependency.srcAccessMask = vk::AccessFlagBits::eMemoryRead;
  input_dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead |
                                   vk::AccessFlagBits::eColorAttachmentWrite;
  input_dependency.dependencyFlags = vk::DependencyFlagBits::eByRegion;

  // The second dependency describes the transition from the initial to final
  // layout.
  output_dependency.srcSubpass = 0;  // our sole subpass
  output_dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
  output_dependency.srcStageMask =
      vk::PipelineStageFlagBits::eColorAttachmentOutput;
  output_dependency.dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
  output_dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentRead |
                                    vk::AccessFlagBits::eColorAttachmentWrite;
  output_dependency.dstAccessMask = vk::AccessFlagBits::eMemoryRead;
  output_dependency.dependencyFlags = vk::DependencyFlagBits::eByRegion;

  // We're almost ready to create the render-passes... we just need to fill in
  // some final values that differ between the passes.
  vk::RenderPassCreateInfo info;
  info.attachmentCount = kAttachmentCount;
  info.pAttachments = attachments;
  info.subpassCount = 1;
  info.pSubpasses = &subpass;
  info.dependencyCount = kDependencyCount;
  info.pDependencies = dependencies;

  // Create the depth-prepass RenderPass.
  color_attachment.format = pre_pass_color_format;
  color_attachment.samples = vk::SampleCountFlagBits::e1;
  color_attachment.loadOp = vk::AttachmentLoadOp::eDontCare;
  color_attachment.storeOp = vk::AttachmentStoreOp::eDontCare;
  color_attachment.initialLayout = vk::ImageLayout::eUndefined;
  color_attachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
  depth_attachment.samples = vk::SampleCountFlagBits::e1;
  depth_attachment.loadOp = vk::AttachmentLoadOp::eClear;
  depth_attachment.storeOp = vk::AttachmentStoreOp::eStore;
  depth_attachment.initialLayout = vk::ImageLayout::eUndefined;
  depth_attachment.finalLayout =
      vk::ImageLayout::eDepthStencilAttachmentOptimal;
  depth_prepass_ = ESCHER_CHECKED_VK_RESULT(device_.createRenderPass(info));

  // Create the illumination RenderPass.
  color_attachment.format = lighting_pass_color_format;
  color_attachment.samples = vk::SampleCountFlagBits::e4;
  color_attachment.loadOp = vk::AttachmentLoadOp::eClear;
  // TODO: necessary to store if we resolve as part of the render-pass?
  color_attachment.storeOp = vk::AttachmentStoreOp::eStore;
  color_attachment.initialLayout = vk::ImageLayout::eUndefined;
  color_attachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
  depth_attachment.samples = vk::SampleCountFlagBits::e4;
  depth_attachment.loadOp = vk::AttachmentLoadOp::eClear;
  depth_attachment.storeOp = vk::AttachmentStoreOp::eDontCare;
  depth_attachment.initialLayout = vk::ImageLayout::eUndefined;
  depth_attachment.finalLayout =
      vk::ImageLayout::eDepthStencilAttachmentOptimal;
  lighting_pass_ = ESCHER_CHECKED_VK_RESULT(device_.createRenderPass(info));
}

}  // namespace impl
}  // namespace escher
