// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <vector>

#include "escher/forward_declarations.h"
#include "escher/geometry/types.h"

namespace escher {

// Tessellate a circle.  The coarsest circle (i.e. subdivisions == 0) is a
// square; increasing the number of subdivisions doubles the number of vertices.
MeshPtr NewCircleMesh(MeshBuilderFactory* factory,
                      const MeshSpec& spec,
                      int subdivisions,
                      vec2 center,
                      float radius,
                      float offset_magnitude = 0.f);

// Tessellate a basic rectangle
MeshPtr NewSimpleRectangleMesh(MeshBuilderFactory* factory);

// Tessellate a rectangle with multiple vertices along the top and bottom edges.
// Increasing subdivisions by 1 doubles the number of vertices. If the spec
// has kPositionOffset, the top offset points up and the bottom points down.
MeshPtr NewRectangleMesh(MeshBuilderFactory* factory,
                         const MeshSpec& spec,
                         int subdivisions,
                         vec2 size,
                         vec2 top_left = vec2(0.f, 0.f),
                         float top_offset_magnitude = 0.f,
                         float bottom_offset_magnitude = 0.f);

// Tessellate a ring whose area is bounded by an inner and an outer circle.
// Increasing subdivisions by 1 doubles the number of vertices.  If the spec
// has kPositionOffset, the outer offset points outward (away from the center of
// the ring) and the inner offset points inward (toward the center of the ring).
MeshPtr NewRingMesh(MeshBuilderFactory* factory,
                    const MeshSpec& spec,
                    int subdivisions,
                    vec2 center,
                    float outer_radius,
                    float inner_radius,
                    float outer_offset_magnitude = 0.f,
                    float inner_offset_magnitude = 0.f);

// Tessellate a full-screen mesh.  The returned mesh has only position and UV
// coordinates.
MeshPtr NewFullScreenMesh(MeshBuilderFactory* factory);

}  // namespace escher
