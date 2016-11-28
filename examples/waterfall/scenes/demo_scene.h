// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "escher/escher.h"

#include "examples/waterfall/scenes/scene.h"

class DemoScene : public Scene {
 public:
  DemoScene(escher::VulkanContext* vulkan_context, escher::Escher* escher);
  ~DemoScene();

  void Init(escher::Stage* stage) override;

  escher::Model* Update(const escher::Stopwatch& stopwatch,
                        uint64_t frame_count,
                        escher::Stage* stage) override;

 private:
  std::unique_ptr<escher::Model> model_;

  escher::MaterialPtr purple_;

  FTL_DISALLOW_COPY_AND_ASSIGN(DemoScene);
};
