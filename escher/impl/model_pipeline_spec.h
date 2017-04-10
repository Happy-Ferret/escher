// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "escher/scene/shape.h"
#include "escher/shape/mesh_spec.h"

namespace escher {
namespace impl {

// Used to look up cached Vulkan pipelines that are compatible with the params.
#pragma pack(push, 1)  // As required by escher::Hash<ModelPipelineSpec>
struct ModelPipelineSpec {
  enum class ClipperState {
    // The current object clips subsequent objects to its bounds, until the
    // original object is rendered again with |kEndClipChildren|.
    kBeginClipChildren = 1,
    // Clean up the clip region established by |kBeginClipChildren.
    kEndClipChildren,
    // This object rendered by this pipeline has no children to clip.
    kNoClipChildren
  };

  MeshSpec mesh_spec;
  ShapeModifiers shape_modifiers;
  // TODO: For now, there is only 1 material, so the ModelPipelineSpec doesn't
  // bother to mention anything about it.
  uint32_t sample_count = 1;
  ClipperState clipper_state = ClipperState::kNoClipChildren;
  bool is_clippee = false;
  // TODO: this is a hack.
  bool use_depth_prepass = true;
};
#pragma pack(pop)

// Inline function definitions.

inline bool operator==(const ModelPipelineSpec& spec1,
                       const ModelPipelineSpec& spec2) {
  return spec1.mesh_spec == spec2.mesh_spec &&
         spec1.shape_modifiers == spec2.shape_modifiers &&
         spec1.sample_count == spec2.sample_count &&
         spec1.clipper_state == spec2.clipper_state &&
         spec1.is_clippee == spec2.is_clippee &&
         spec1.use_depth_prepass == spec2.use_depth_prepass;
}

inline bool operator!=(const ModelPipelineSpec& spec1,
                       const ModelPipelineSpec& spec2) {
  return !(spec1 == spec2);
}

}  // namespace impl

// Debugging.
ESCHER_DEBUG_PRINTABLE(impl::ModelPipelineSpec);

}  // namespace escher
