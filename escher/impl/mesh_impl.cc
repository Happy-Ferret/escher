// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "escher/impl/mesh_impl.h"

#include "escher/impl/mesh_manager.h"

namespace escher {
namespace impl {

MeshImpl::MeshImpl(MeshSpec spec,
                   uint32_t num_vertices,
                   uint32_t num_indices,
                   MeshManager* manager,
                   Buffer vertex_buffer,
                   Buffer index_buffer,
                   const MeshSpecImpl& spec_impl)
    // TODO: shouldn't pass nullptr as first argument.  Leaving for now, because
    // we might remove the EscherImpl field from Resource.
    : Mesh(nullptr, spec, num_vertices, num_indices),
      manager_(manager),
      vertex_buffer_(std::move(vertex_buffer)),
      index_buffer_(std::move(index_buffer)),
      spec_impl_(spec_impl) {
  manager_->IncrementMeshCount();
}

MeshImpl::~MeshImpl() {
  manager_->DecrementMeshCount();
}

}  // namespace impl
}  // namespace escher
