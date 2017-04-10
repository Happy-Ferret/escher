// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "escher/escher.h"

#include "examples/waterfall/scenes/scene.h"

class UberScene : public Scene {
 public:
  UberScene(Demo* demo);
  ~UberScene();

  void Init(escher::Stage* stage) override;

  escher::Model* Update(const escher::Stopwatch& stopwatch,
                        uint64_t frame_count,
                        escher::Stage* stage) override;

 private:
  std::unique_ptr<escher::Model> model_;

  escher::MaterialPtr blue_;
  escher::MaterialPtr red_;
  escher::MaterialPtr purple_;
  escher::MaterialPtr bg_;

  escher::MeshPtr ring_mesh1_;
  escher::MeshPtr ring_mesh2_;
  escher::MeshPtr ring_mesh3_;
  escher::MeshPtr ring_mesh4_;

  FTL_DISALLOW_COPY_AND_ASSIGN(UberScene);
};
