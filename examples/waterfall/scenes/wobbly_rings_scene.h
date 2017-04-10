// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "escher/escher.h"

#include "escher/geometry/types.h"
#include "examples/waterfall/scenes/scene.h"

using escher::vec3;

class WobblyRingsScene : public Scene {
 public:
  WobblyRingsScene(Demo* demo,
                   vec3 clear_color,
                   vec3 ring1_color,
                   vec3 ring2_color,
                   vec3 ring3_color,
                   vec3 circle_color,
                   vec3 checkerboard_color);
  ~WobblyRingsScene();

  void Init(escher::Stage* stage) override;

  escher::Model* Update(const escher::Stopwatch& stopwatch,
                        uint64_t frame_count,
                        escher::Stage* stage) override;

 private:
  std::unique_ptr<escher::Model> model_;

  vec3 clear_color_;
  escher::MeshPtr ring_mesh1_;
  escher::MeshPtr ring_mesh2_;
  escher::MeshPtr ring_mesh3_;
  escher::MeshPtr wobbly_rect_mesh_;
  escher::MaterialPtr circle_color_;
  escher::MaterialPtr clip_color_;
  escher::MaterialPtr ring1_color_;
  escher::MaterialPtr ring2_color_;
  escher::MaterialPtr ring3_color_;
  escher::MaterialPtr checkerboard_material_;

  FTL_DISALLOW_COPY_AND_ASSIGN(WobblyRingsScene);
};
