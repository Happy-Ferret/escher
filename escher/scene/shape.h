// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "escher/geometry/types.h"
#include "escher/shape/mesh.h"
#include "escher/util/debug_print.h"
#include "escher/scene/shape_modifier.h"

#include "ftl/logging.h"

namespace escher {

// Describes a planar shape primitive to be drawn.
class Shape {
 public:
  enum class Type { kRect, kCircle, kMesh, kNone };

  explicit Shape(Type type, ShapeModifiers modifiers = ShapeModifiers());
  explicit Shape(MeshPtr mesh, ShapeModifiers modifiers = ShapeModifiers());
  ~Shape();

  Type type() const { return type_; }
  ShapeModifiers modifiers() const { return modifiers_; }
  void set_mesh(MeshPtr mesh);
  void set_modifiers(ShapeModifiers modifiers) { modifiers_ = modifiers; }
  void remove_modifier(ShapeModifier modifier) { modifiers_ &= ~modifier; }

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

}  // namespace escher
