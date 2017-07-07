// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "escher/resources/resource_recycler.h"

#include "escher/escher.h"
#include "escher/impl/escher_impl.h"

namespace escher {

ResourceRecycler::ResourceRecycler(Escher* escher)
    : ResourceRecycler(escher->impl_.get()) {}

ResourceRecycler::ResourceRecycler(impl::EscherImpl* escher)
    : ResourceManager(escher->vulkan_context()), escher_(escher) {
  Register(escher_->command_buffer_sequencer());
}

ResourceRecycler::~ResourceRecycler() {
  FTL_DCHECK(unused_resources_.empty());
  Unregister(escher_->command_buffer_sequencer());
}

void ResourceRecycler::OnReceiveOwnable(std::unique_ptr<Resource> resource) {
  if (resource->sequence_number() <= last_finished_sequence_number_) {
    // Recycle immediately.
    RecycleResource(std::move(resource));
  } else {
    // Defer recycling.
    unused_resources_[resource.get()] = std::move(resource);
  }
}

void ResourceRecycler::OnCommandBufferFinished(uint64_t sequence_number) {
  FTL_DCHECK(sequence_number > last_finished_sequence_number_);
  last_finished_sequence_number_ = sequence_number;

  // The sequence number allows us to find all unused resources that are no
  // longer referenced by a pending command-buffer; destroy these.
  auto it = unused_resources_.begin();
  while (it != unused_resources_.end()) {
    if (it->second->sequence_number() <= last_finished_sequence_number_) {
      RecycleResource(std::move(it->second));
      it = unused_resources_.erase(it);
    } else {
      ++it;
    }
  }
}

}  // namespace escher
