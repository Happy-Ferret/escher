// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "escher/impl/compute_shader.h"

#include "escher/impl/command_buffer.h"
#include "escher/impl/glsl_compiler.h"
#include "escher/impl/vk/pipeline.h"
#include "escher/impl/vk/pipeline_spec.h"
#include "escher/impl/vulkan_utils.h"
#include "escher/renderer/texture.h"

namespace escher {
namespace impl {

namespace {

// Used by ComputeShader constructor.
inline std::vector<vk::DescriptorSetLayoutBinding> CreateLayoutBindings(
    const std::vector<vk::ImageLayout>& layouts) {
  std::vector<vk::DescriptorSetLayoutBinding> result;
  for (uint32_t index = 0; index < layouts.size(); ++index) {
    vk::DescriptorType descriptor_type;
    switch (layouts[index]) {
      case vk::ImageLayout::eShaderReadOnlyOptimal:
        descriptor_type = vk::DescriptorType::eCombinedImageSampler;
        break;
      case vk::ImageLayout::eGeneral:
        descriptor_type = vk::DescriptorType::eStorageImage;
        break;
      default:
        FTL_LOG(ERROR) << "unsupported layout: "
                       << vk::to_string(layouts[index]);
        FTL_CHECK(false);
        descriptor_type = vk::DescriptorType::eStorageImage;
    }
    result.push_back({index, descriptor_type, 1,
                      vk::ShaderStageFlagBits::eCompute, nullptr});
  }
  return result;
}

// Used by ComputeShader constructor.
inline vk::DescriptorSetLayoutCreateInfo CreateDescriptorSetLayoutCreateInfo(
    const std::vector<vk::DescriptorSetLayoutBinding>& bindings) {
  vk::DescriptorSetLayoutCreateInfo info;
  info.bindingCount = static_cast<uint32_t>(bindings.size());
  info.pBindings = bindings.data();
  return info;
}

// Used by ComputeShader constructor.
PipelinePtr CreatePipeline(vk::Device device,
                           vk::DescriptorSetLayout descriptor_set_layout,
                           uint32_t push_constants_size,
                           const char* source_code,
                           GlslToSpirvCompiler* compiler) {
  vk::ShaderModule module;
  {
    SpirvData spirv = compiler
                          ->Compile(vk::ShaderStageFlagBits::eCompute,
                                    {{source_code}}, std::string(), "main")
                          .get();

    vk::ShaderModuleCreateInfo module_info;
    module_info.codeSize = spirv.size() * sizeof(uint32_t);
    module_info.pCode = spirv.data();
    module = ESCHER_CHECKED_VK_RESULT(device.createShaderModule(module_info));
  }

  vk::PushConstantRange push_constants;
  push_constants.stageFlags = vk::ShaderStageFlagBits::eCompute;
  push_constants.offset = 0;
  push_constants.size = push_constants_size;

  vk::PipelineLayoutCreateInfo pipeline_layout_info;
  pipeline_layout_info.setLayoutCount = 1;
  pipeline_layout_info.pSetLayouts = &descriptor_set_layout;
  pipeline_layout_info.pushConstantRangeCount = push_constants_size > 0 ? 1 : 0;
  pipeline_layout_info.pPushConstantRanges =
      push_constants_size > 0 ? &push_constants : nullptr;

  auto pipeline_layout = ftl::MakeRefCounted<PipelineLayout>(
      device, ESCHER_CHECKED_VK_RESULT(
                  device.createPipelineLayout(pipeline_layout_info, nullptr)));

  vk::PipelineShaderStageCreateInfo shader_stage_info;
  shader_stage_info.stage = vk::ShaderStageFlagBits::eCompute;
  shader_stage_info.module = module;
  shader_stage_info.pName = "main";

  vk::ComputePipelineCreateInfo pipeline_info;
  pipeline_info.stage = shader_stage_info;
  pipeline_info.layout = pipeline_layout->get();

  vk::Pipeline vk_pipeline = ESCHER_CHECKED_VK_RESULT(
      device.createComputePipeline(nullptr, pipeline_info));
  auto pipeline = ftl::MakeRefCounted<Pipeline>(
      device, vk_pipeline, pipeline_layout, PipelineSpec());

  device.destroyShaderModule(module);

  return pipeline;
}

}  // namespace

ComputeShader::ComputeShader(vk::Device device,
                             std::vector<vk::ImageLayout> layouts,
                             size_t push_constants_size,
                             const char* source_code,
                             GlslToSpirvCompiler* compiler)
    : device_(device),
      descriptor_set_layout_bindings_(CreateLayoutBindings(layouts)),
      descriptor_set_layout_create_info_(
          CreateDescriptorSetLayoutCreateInfo(descriptor_set_layout_bindings_)),
      push_constants_size_(static_cast<uint32_t>(push_constants_size)),
      pool_(device, descriptor_set_layout_create_info_),
      pipeline_(CreatePipeline(device,
                               pool_.layout(),
                               push_constants_size_,
                               source_code,
                               compiler)) {
  FTL_DCHECK(push_constants_size == push_constants_size_);  // detect overflow
  descriptor_image_info_.reserve(layouts.size());
  descriptor_set_writes_.reserve(layouts.size());
  for (uint32_t index = 0; index < layouts.size(); ++index) {
    // The other fields will be filled out during each call to Dispatch().
    vk::DescriptorImageInfo image_info;
    image_info.imageLayout = layouts[index];
    descriptor_image_info_.push_back(image_info);

    vk::WriteDescriptorSet write;
    write.dstArrayElement = 0;
    write.descriptorType =
        descriptor_set_layout_bindings_[index].descriptorType;
    write.descriptorCount = 1;
    write.dstBinding = index;
    write.pImageInfo = &descriptor_image_info_[index];
    descriptor_set_writes_.push_back(write);
  }
}

ComputeShader::~ComputeShader() {}

void ComputeShader::Dispatch(std::vector<TexturePtr> textures,
                             CommandBuffer* command_buffer,
                             uint32_t x,
                             uint32_t y,
                             uint32_t z,
                             const void* push_constants) {
  // Push constants must be provided if and only if the pipeline is configured
  // to use them.
  FTL_DCHECK((push_constants_size_ == 0) == (push_constants == nullptr));

  auto descriptor_set = pool_.Allocate(1, command_buffer)->get(0);
  for (uint32_t i = 0; i < textures.size(); ++i) {
    descriptor_set_writes_[i].dstSet = descriptor_set;
    descriptor_image_info_[i].imageView = textures[i]->image_view();
    descriptor_image_info_[i].sampler = textures[i]->sampler();
    textures[i]->KeepAlive(command_buffer);
  }
  device_.updateDescriptorSets(
      static_cast<uint32_t>(descriptor_set_writes_.size()),
      descriptor_set_writes_.data(), 0, nullptr);

  auto vk_command_buffer = command_buffer->get();
  auto vk_pipeline_layout = pipeline_->layout();

  if (push_constants) {
    vk_command_buffer.pushConstants(vk_pipeline_layout,
                                    vk::ShaderStageFlagBits::eCompute, 0,
                                    push_constants_size_, push_constants);
  }
  vk_command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute,
                                 pipeline_->get());
  vk_command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                                       vk_pipeline_layout, 0, 1,
                                       &descriptor_set, 0, nullptr);
  vk_command_buffer.dispatch(x, y, z);
}

}  // namespace impl
}  // namespace escher
