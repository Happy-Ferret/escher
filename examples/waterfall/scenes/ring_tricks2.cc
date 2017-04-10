// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/waterfall/scenes/ring_tricks2.h"

#include "escher/geometry/tessellation.h"
#include "escher/geometry/types.h"
#include "escher/material/material.h"
#include "escher/renderer/image.h"
#include "escher/scene/model.h"
#include "escher/scene/stage.h"
#include "escher/shape/modifier_wobble.h"
#include "escher/util/stopwatch.h"
#include "escher/vk/vulkan_context.h"

using escher::vec2;
using escher::vec3;
using escher::MeshAttribute;
using escher::MeshSpec;
using escher::Object;
using escher::ShapeModifier;

RingTricks2::RingTricks2(Demo* demo) : Scene(demo) {}

void RingTricks2::Init(escher::Stage* stage) {
  red_ = ftl::MakeRefCounted<escher::Material>();
  bg_ = ftl::MakeRefCounted<escher::Material>();
  color1_ = ftl::MakeRefCounted<escher::Material>();
  color2_ = ftl::MakeRefCounted<escher::Material>();
  red_->set_color(vec3(0.98f, 0.15f, 0.15f));
  bg_->set_color(vec3(0.8f, 0.8f, 0.8f));
  color1_->set_color(vec3(63.f / 255.f, 138.f / 255.f, 153.f / 255.f));
  color2_->set_color(vec3(143.f / 255.f, 143.f / 255.f, 143.f / 255.f));

  // Create meshes for fancy wobble effect.
  MeshSpec spec{MeshAttribute::kPosition | MeshAttribute::kPositionOffset |
                MeshAttribute::kPerimeterPos | MeshAttribute::kUV};
  ring_mesh1_ = escher::NewRingMesh(escher(), spec, 8, vec2(0.f, 0.f), 285.f,
                                    265.f, 18.f, -15.f);
}

RingTricks2::~RingTricks2() {}

escher::Model* RingTricks2::Update(const escher::Stopwatch& stopwatch,
                                   uint64_t frame_count,
                                   escher::Stage* stage) {
  float current_time_sec = stopwatch.GetElapsedSeconds();

  float screen_width = stage->viewing_volume().width();
  float screen_height = stage->viewing_volume().height();
  float min_height = 5.f;
  float max_height = 80.f;
  float elevation_range = max_height - min_height;

  std::vector<Object> objects;

  // Orbiting circle1
  float circle1_orbit_radius = 275.f;
  float circle1_x_pos = sin(current_time_sec * 1.f) * circle1_orbit_radius +
                        (screen_width * 0.5f);
  float circle1_y_pos = cos(current_time_sec * 1.f) * circle1_orbit_radius +
                        (screen_height * 0.5f);
  Object circle1(
      Object::NewCircle(vec2(circle1_x_pos, circle1_y_pos), 60.f, 35.f, red_));
  objects.push_back(circle1);

  // Orbiting circle1
  float circle2_orbit_radius = 120.f;
  float circle2_x_pos =
      sin(current_time_sec * 2.f) * circle2_orbit_radius + circle1_x_pos;
  float circle2_y_pos =
      cos(current_time_sec * 2.f) * circle2_orbit_radius + circle1_y_pos;
  float circle2_elevation =
      (cos(current_time_sec * 1.5f) * 0.5 + 0.5) * elevation_range + min_height;
  Object circle2(Object::NewCircle(vec2(circle2_x_pos, circle2_y_pos), 30.f,
                                   circle2_elevation, color1_));
  objects.push_back(circle2);

  // Create the ring that will do the fancy trick
  vec3 inner_ring_pos(screen_width * 0.5f, screen_height * 0.5f, 30.f);
  Object inner_ring(ring_mesh1_, inner_ring_pos, color2_);
  inner_ring.set_shape_modifiers(ShapeModifier::kWobble);
  objects.push_back(inner_ring);

  // Create our background plane
  Object bg_plane(Object::NewRect(vec2(0.f, 0.f),
                                  vec2(screen_width, screen_height), 0.f, bg_));
  objects.push_back(bg_plane);

  // Create the Model
  model_ = std::unique_ptr<escher::Model>(new escher::Model(objects));
  model_->set_time(current_time_sec);

  return model_.get();
}
