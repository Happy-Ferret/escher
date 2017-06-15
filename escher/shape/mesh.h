// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <map>

#include "escher/forward_declarations.h"
#include "escher/resources/waitable_resource.h"
#include "escher/shape/mesh_spec.h"

namespace escher {

// Immutable container for vertex indices and attribute data required to render
// a triangle mesh.
class Mesh : public WaitableResource {
 public:
  static const ResourceTypeInfo kTypeInfo;
  const ResourceTypeInfo& type_info() const override { return kTypeInfo; }

  Mesh(ResourceRecycler* resource_recycler,
       MeshSpec spec,
       uint32_t num_vertices,
       uint32_t num_indices,
       BufferPtr vertex_buffer,
       BufferPtr index_buffer,
       vk::DeviceSize vertex_buffer_offset = 0,
       vk::DeviceSize index_buffer_offset = 0);

  ~Mesh() override;

  const MeshSpec& spec() { return spec_; }
  uint32_t num_vertices() const { return num_vertices_; }
  uint32_t num_indices() const { return num_indices_; }
  vk::Buffer vertex_buffer() const { return vertex_buffer_; }
  vk::Buffer index_buffer() const { return index_buffer_; }
  vk::DeviceSize vertex_buffer_offset() const { return vertex_buffer_offset_; }
  vk::DeviceSize index_buffer_offset() const { return index_buffer_offset_; }

 private:
  const MeshSpec spec_;
  const uint32_t num_vertices_;
  const uint32_t num_indices_;
  const vk::Buffer vertex_buffer_;
  const vk::Buffer index_buffer_;
  const BufferPtr vertex_buffer_ptr_;
  const BufferPtr index_buffer_ptr_;
  const vk::DeviceSize vertex_buffer_offset_;
  const vk::DeviceSize index_buffer_offset_;

  FTL_DISALLOW_COPY_AND_ASSIGN(Mesh);
};

typedef ftl::RefPtr<Mesh> MeshPtr;

}  // namespace escher
