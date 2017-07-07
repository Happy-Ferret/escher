// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <vulkan/vulkan.hpp>

#include "escher/impl/model_pipeline_spec.h"
#include "ftl/macros.h"

namespace escher {
namespace impl {

class ModelPipeline {
 public:
  ModelPipeline(const ModelPipelineSpec& spec,
                vk::Device device,
                vk::Pipeline pipeline,
                vk::PipelineLayout pipeline_layout);
  ~ModelPipeline();

  vk::Pipeline pipeline() const { return pipeline_; }
  vk::PipelineLayout pipeline_layout() const { return pipeline_layout_; }

  // Return true if this pipeline was created with
  // VK_DYNAMIC_STATE_STENCIL_REFERENCE.
  bool HasDynamicStencilState() const { return spec_.is_clippee; }

 private:
  friend class ModelPipelineCache;

  ModelPipelineSpec spec_;
  vk::Device device_;
  vk::Pipeline pipeline_;
  vk::PipelineLayout pipeline_layout_;

  FTL_DISALLOW_COPY_AND_ASSIGN(ModelPipeline);
};

}  // namespace impl
}  // namespace escher
