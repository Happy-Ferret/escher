// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "escher/impl/model_data.h"

#include "escher/impl/gpu_allocator.h"
#include "escher/impl/model_uniform_writer.h"
#include "escher/impl/render_frame.h"
#include "escher/impl/vulkan_utils.h"

namespace escher {
namespace impl {

ModelData::ModelData(vk::Device device, GpuAllocator* allocator)
    : device_(device),
      allocator_(allocator),
      per_model_layout_(NewPerModelLayout()),
      per_object_layout_(NewPerObjectLayout()) {}

ModelData::~ModelData() {
  device_.destroyDescriptorSetLayout(per_model_layout_);
  device_.destroyDescriptorSetLayout(per_object_layout_);
}

vk::DescriptorSetLayout ModelData::NewPerModelLayout() {
  vk::DescriptorSetLayoutBinding binding;
  binding.binding = 0;
  binding.descriptorType = vk::DescriptorType::eUniformBuffer;
  binding.descriptorCount = 1;
  binding.stageFlags |= vk::ShaderStageFlagBits::eFragment;

  vk::DescriptorSetLayoutCreateInfo info;
  info.bindingCount = 1;
  info.pBindings = &binding;

  return ESCHER_CHECKED_VK_RESULT(device_.createDescriptorSetLayout(info));
}

vk::DescriptorSetLayout ModelData::NewPerObjectLayout() {
  vk::DescriptorSetLayoutBinding binding;
  binding.binding = 0;
  binding.descriptorType = vk::DescriptorType::eUniformBuffer;
  binding.descriptorCount = 1;
  binding.stageFlags |= vk::ShaderStageFlagBits::eFragment;
  binding.stageFlags |= vk::ShaderStageFlagBits::eVertex;

  vk::DescriptorSetLayoutCreateInfo info;
  info.bindingCount = 1;
  info.pBindings = &binding;

  return ESCHER_CHECKED_VK_RESULT(device_.createDescriptorSetLayout(info));
}

ModelUniformWriter* ModelData::GetWriterWithCapacity(
    RenderFrame* frame,
    size_t max_object_count,
    float overallocate_percent) {
  auto ptr = writers_[frame].get();
  if (!ptr || ptr->capacity() < max_object_count) {
    // Create a new writer with at least the required capacity.
    uint32_t capacity =
        static_cast<uint32_t>(max_object_count * (1.f + overallocate_percent));
    FTL_CHECK(capacity >= max_object_count);
    auto writer = std::make_unique<ModelUniformWriter>(
        device_, allocator_, capacity, per_model_layout_, per_object_layout_);
    ptr = writer.get();
    writers_[frame] = std::move(writer);
  }
  ptr->BecomeWritable();
  return ptr;
}

}  // namespace impl
}  // namespace escher
