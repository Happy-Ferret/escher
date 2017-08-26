// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "escher/shape/mesh_spec.h"

#include "escher/geometry/types.h"
#include "lib/ftl/logging.h"

namespace escher {

size_t GetMeshAttributeSize(MeshAttribute attr) {
  switch (attr) {
    case MeshAttribute::kPosition2D:
      return sizeof(vec2);
    case MeshAttribute::kPosition3D:
      return sizeof(vec3);
    case MeshAttribute::kPositionOffset:
      return sizeof(vec2);
    case MeshAttribute::kUV:
      return sizeof(vec2);
    case MeshAttribute::kPerimeterPos:
      return sizeof(float);
    case MeshAttribute::kStride:
      FTL_CHECK(false);
      return 0;
  }
}

size_t MeshSpec::GetAttributeOffset(MeshAttribute flag) const {
  FTL_DCHECK(flags & flag || flag == MeshAttribute::kStride);
  size_t offset = 0;

  if (flag == MeshAttribute::kPosition2D) {
    return offset;
  } else if (flags & MeshAttribute::kPosition2D) {
    offset += sizeof(vec2);
  }

  if (flag == MeshAttribute::kPosition3D) {
    return offset;
  } else if (flags & MeshAttribute::kPosition3D) {
    offset += sizeof(vec3);
  }

  if (flag == MeshAttribute::kPositionOffset) {
    return offset;
  } else if (flags & MeshAttribute::kPositionOffset) {
    offset += sizeof(vec2);
  }

  if (flag == MeshAttribute::kUV) {
    return offset;
  } else if (flags & MeshAttribute::kUV) {
    offset += sizeof(vec2);
  }

  if (flag == MeshAttribute::kPerimeterPos) {
    return offset;
  } else if (flags & MeshAttribute::kPerimeterPos) {
    offset += sizeof(float);
  }

  FTL_DCHECK(flag == MeshAttribute::kStride);
  return offset;
}

bool MeshSpec::IsValid() const {
  if (flags & MeshAttribute::kPosition2D) {
    // Meshes cannot have both 2D and 3D positions.
    return !(flags & MeshAttribute::kPosition3D);
  } else if (flags & MeshAttribute::kPosition3D) {
    // Position-offset and perimeter attributes are only allowed for 2D meshes.
    return !(flags & MeshAttribute::kPositionOffset) &&
           !(flags & MeshAttribute::kPerimeterPos);
  } else {
    // All meshes must have either 2D or 3D positions.
    return false;
  }
}

}  // namespace escher
