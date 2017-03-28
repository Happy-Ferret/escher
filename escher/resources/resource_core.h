// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>

#include "escher/vk/vulkan_context.h"
#include "ftl/logging.h"

namespace escher {

// Defined below.
class ResourceCore;

// ResourceCoreManager is responsible for deciding whether to reuse or destroy
// ResourceCores that are returned to it.  The only restriction is that the
// manager must wait until it is "safe" to destroy the core; the definition of
// safety depends on the context, but typically means something like not
// destroying cores while they are used by pending Vulkan command-buffers.
//
// Not thread-safe.
class ResourceCoreManager {
 public:
  explicit ResourceCoreManager(const VulkanContext& context);
  virtual ~ResourceCoreManager();
  virtual void ReceiveResourceCore(std::unique_ptr<ResourceCore> core) = 0;

  const VulkanContext& vulkan_context() const { return vulkan_context_; }
  vk::Device device() const { return vulkan_context_.device; }

 private:
  friend class ResourceCore;

  void IncrementResourceCount() { ++resource_count_; }
  void DecrementResourceCount() { --resource_count_; }

  VulkanContext vulkan_context_;
  uint64_t resource_count_ = 0;

  FTL_DISALLOW_COPY_AND_ASSIGN(ResourceCoreManager);
};

// Each ResourceCore is owned by a single Resource.  When that Resource dies,
// the ResourceCore is passed to its ResourceCoreManager, which decides what to
// do.  For example, a common use-case is to keep Vulkan resources alive as long
// as they are used by a pending command-buffer.
//
// Not thread-safe.
class ResourceCore {
 public:
  explicit ResourceCore(ResourceCoreManager* manager);
  virtual ~ResourceCore();

  uint64_t sequence_number() const { return sequence_number_; }

  const VulkanContext& vulkan_context() const {
    return manager_->vulkan_context();
  }

 private:
  friend class Resource2;
  void set_sequence_number(uint64_t sequence_number) {
    FTL_DCHECK(sequence_number >= sequence_number_);
    sequence_number_ = sequence_number;
  }

  ResourceCoreManager* manager_;
  uint64_t sequence_number_ = 0;

  FTL_DISALLOW_COPY_AND_ASSIGN(ResourceCore);
};

}  // namespace escher
