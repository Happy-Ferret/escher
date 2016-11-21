// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "escher/shape/mesh.h"
#include "ftl/memory/ref_counted.h"

namespace escher {

// MeshBuilder is used by Escher clients to generate Meshes.  Clients should
// obtain one via Esher::NewMeshBuilder(), repeatedly call AddVertex() and
// AddIndex() to add data for the Mesh, and then call Build() once all data has
// been added.
class MeshBuilder : public ftl::RefCountedThreadSafe<MeshBuilder> {
 public:
  // Return a mesh constructed from the indices and vertices added by AddIndex()
  // and AddVertex(), respectively.  This can only be called once.
  virtual MeshPtr Build() = 0;

  // Copy the index into the staging buffer, so that it will be uploaded to the
  // GPU when Build() is called.
  MeshBuilder& AddIndex(uint32_t index);

  // Copy |size| bytes of data to the staging buffer; this data represents a
  // single vertex.
  MeshBuilder& AddVertexData(const void* ptr, size_t size);

  // Wrap AddVertexData() to automatically obtain the size from the vertex.
  template <typename VertexT>
  MeshBuilder& AddVertex(const VertexT& v);

  // Return the size of a vertex for the given mesh-spec.
  size_t vertex_stride() const { return vertex_stride_; }

  // Return the byte-offset of the attribute within each vertex.
  virtual size_t GetAttributeOffset(MeshAttributeFlagBits flag) = 0;

 protected:
  MeshBuilder(size_t max_vertex_count,
              size_t max_index_count,
              size_t vertex_stride,
              uint8_t* vertex_staging_buffer,
              uint32_t* index_staging_buffer);
  FRIEND_REF_COUNTED_THREAD_SAFE(MeshBuilder);
  virtual ~MeshBuilder();

  const size_t max_vertex_count_;
  const size_t max_index_count_;
  const size_t vertex_stride_;
  uint8_t* vertex_staging_buffer_;
  uint32_t* index_staging_buffer_;
  size_t vertex_count_ = 0;
  size_t index_count_ = 0;

  FTL_DISALLOW_COPY_AND_ASSIGN(MeshBuilder);
};

typedef ftl::RefPtr<MeshBuilder> MeshBuilderPtr;

// Inline function definitions.

inline MeshBuilder& MeshBuilder::AddIndex(uint32_t index) {
  FTL_DCHECK(index_count_ < max_index_count_);
  index_staging_buffer_[index_count_++] = index;
  return *this;
}

inline MeshBuilder& MeshBuilder::AddVertexData(const void* ptr, size_t size) {
  FTL_DCHECK(vertex_count_ < max_vertex_count_);
  FTL_DCHECK(size <= vertex_stride_);
  size_t offset = vertex_stride_ * vertex_count_++;
  memcpy(vertex_staging_buffer_ + offset, ptr, size);
  return *this;
}

template <typename VertexT>
MeshBuilder& MeshBuilder::AddVertex(const VertexT& v) {
  AddVertexData(&v, sizeof(VertexT));
  return *this;
}

}  // namespace escher
