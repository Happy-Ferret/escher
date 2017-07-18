// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "escher/geometry/types.h"
#include "escher/shape/mesh.h"
#include "escher/util/debug_print.h"

#include "ftl/logging.h"

namespace escher {

// Set of flags that specify modifications that should be made to a shape.
// The specified modifiers must be compatible with each other, and the mesh
// attribute layout (this is enforced by DCHECK, so go ahead and try).
enum class ShapeModifier {
  // Adds a sine-wave "wobble" to the shape's vertex shader.
  kWobble = 1,
};

using ShapeModifiers = vk::Flags<ShapeModifier, uint32_t>;

inline ShapeModifiers operator|(ShapeModifier bit0, ShapeModifier bit1) {
  return ShapeModifiers(bit0) | bit1;
}

// Describes a planar shape primitive to be drawn.
class Shape {
 public:
  enum class Type { kRect, kCircle, kMesh, kNone };

  explicit Shape(Type type, ShapeModifiers modifiers = ShapeModifiers());
  explicit Shape(MeshPtr mesh, ShapeModifiers modifiers = ShapeModifiers());
  ~Shape();

  Type type() const { return type_; }
  ShapeModifiers modifiers() const { return modifiers_; }
  void set_modifiers(ShapeModifiers modifiers) { modifiers_ = modifiers; }

  const MeshPtr& mesh() const {
    FTL_DCHECK(type_ == Type::kMesh);
    return mesh_;
  }

  BoundingBox bounding_box() const;

 private:
  Type type_;
  ShapeModifiers modifiers_;
  MeshPtr mesh_;
};

// Debugging.
ESCHER_DEBUG_PRINTABLE(ShapeModifier);
ESCHER_DEBUG_PRINTABLE(ShapeModifiers);

}  // namespace escher
