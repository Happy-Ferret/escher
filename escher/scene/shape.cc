// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "escher/scene/shape.h"

#include "escher/geometry/tessellation.h"

namespace escher {

Shape::Shape(Type type, ShapeModifiers modifiers)
    : type_(type), modifiers_(modifiers) {
  FTL_DCHECK(type != Type::kMesh);
  if (modifiers_ & ShapeModifier::kWobble) {
    FTL_LOG(ERROR) << "ShapeModifier::kWobble only supported for kMesh shapes.";
    FTL_CHECK(false);
  }
}

Shape::Shape(MeshPtr mesh, ShapeModifiers modifiers)
    : type_(Type::kMesh), modifiers_(modifiers), mesh_(mesh) {
  FTL_DCHECK(mesh);
  if (modifiers_ & ShapeModifier::kWobble) {
    const MeshAttributes required =
        MeshAttribute::kPositionOffset | MeshAttribute::kPerimeterPos;
    if (required != (mesh_->spec().flags & required)) {
      FTL_LOG(ERROR) << "ShapeModifier::kWobble requires both kPositionOffset "
                        "and kPerimeterPos";
      FTL_CHECK(false);
    }
  }
}

Shape::~Shape() {}

BoundingBox Shape::bounding_box() const {
  switch (type_) {
    case Type::kRect:
      return BoundingBox({0, 0, 0}, {1, 1, 0});
    case Type::kCircle:
      return BoundingBox({-1, -1, 0}, {1, 1, 0});
    case Type::kMesh:
      return mesh_->bounding_box();
    case Type::kNone:
      return BoundingBox();
  }
}

}  // namespace escher
