// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "escher/escher.h"
#include "escher/examples/common/demo.h"

namespace escher {
class Stopwatch;
}

class Scene {
 public:
  Scene(Demo* demo);
  virtual ~Scene();

  // Convenience method for initializing scene. Use this to create meshes,
  // materials, and other long-lived objects.
  virtual void Init(escher::Stage* stage) = 0;

  // Returns a |Model| for the specified time and frame_count, and gives
  // subclasses a chance to update properties on |stage| (mainly brightness).
  // The returned Model only needs to be valid for the duration of the
  // frame.
  virtual escher::Model* Update(const escher::Stopwatch& stopwatch,
                                uint64_t frame_count,
                                escher::Stage* stage) = 0;

 protected:
  const escher::VulkanContext& vulkan_context() const {
    return demo_->vulkan_context();
  }
  escher::Escher* escher() { return demo_->escher(); }

 private:
  Demo* demo_;

  FTL_DISALLOW_COPY_AND_ASSIGN(Scene);
};
