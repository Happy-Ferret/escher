// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <atomic>
#include <list>
#include <queue>
#include <unordered_map>

#include <vulkan/vulkan.hpp>

#include "escher/impl/gpu_uploader.h"
#include "escher/impl/mesh_impl.h"
#include "escher/shape/mesh_builder.h"
#include "escher/shape/mesh_builder_factory.h"
#include "escher/vk/vulkan_context.h"
#include "ftl/memory/ref_ptr.h"

namespace escher {
namespace impl {
class GpuAllocator;
class GpuUploader;

// Responsible for generating Meshes, tracking their memory use, managing
// synchronization, etc.
//
// Not thread-safe.
class MeshManager : public MeshBuilderFactory {
 public:
  MeshManager(CommandBufferPool* command_buffer_pool,
              GpuAllocator* allocator,
              GpuUploader* uploader);
  ~MeshManager();

  // The returned MeshBuilder is not thread-safe.
  MeshBuilderPtr NewMeshBuilder(const MeshSpec& spec,
                                size_t max_vertex_count,
                                size_t max_index_count) override;

  const MeshSpecImpl& GetMeshSpecImpl(MeshSpec spec);

 private:
  void UpdateBusyResources();

  class MeshBuilder : public escher::MeshBuilder {
   public:
    MeshBuilder(MeshManager* manager,
                const MeshSpec& spec,
                size_t max_vertex_count,
                size_t max_index_count,
                GpuUploader::Writer vertex_writer,
                GpuUploader::Writer index_writer,
                const MeshSpecImpl& spec_impl);
    ~MeshBuilder() override;

    MeshPtr Build() override;

    // Return the byte-offset of the attribute within each vertex.
    size_t GetAttributeOffset(MeshAttribute flag) override;

   private:
    MeshManager* manager_;
    MeshSpec spec_;
    bool is_built_;
    GpuUploader::Writer vertex_writer_;
    GpuUploader::Writer index_writer_;
    const MeshSpecImpl& spec_impl_;
  };

  friend class MeshImpl;
  void IncrementMeshCount() { ++mesh_count_; }
  void DecrementMeshCount() { --mesh_count_; }

  CommandBufferPool* command_buffer_pool_;
  GpuAllocator* allocator_;
  GpuUploader* uploader_;
  vk::Device device_;
  vk::Queue queue_;

  std::unordered_map<MeshSpec, std::unique_ptr<MeshSpecImpl>, MeshSpec::Hash>
      spec_cache_;

  std::atomic<uint32_t> builder_count_;
  std::atomic<uint32_t> mesh_count_;
};

}  // namespace impl
}  // namespace escher
