// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <unordered_map>

#include "escher/impl/command_buffer_sequencer.h"
#include "escher/resources/resource_manager.h"
#include "escher/resources/resource_recycler.h"

namespace escher {

// Simple manager that keeps resources alive until they are no longer referenced
// by a pending command-buffer, then recycles them.  It does this by comparing
// the sequence numbers from a CommandBufferSequencer with the sequence numbers
// of resources that it is keeping alive. The default implementation does not
// recycle resources, instead destroying them as soon as it is safe.
class ResourceRecycler : public ResourceManager,
                         public impl::CommandBufferSequencerListener {
 public:
  explicit ResourceRecycler(Escher* escher);
  explicit ResourceRecycler(impl::EscherImpl* escher);

  virtual ~ResourceRecycler();

 protected:
  impl::EscherImpl* escher() const { return escher_; }

 private:
  // Gives subclasses a chance to recycle the resource. Default implementation
  // immediately destroys resource.
  virtual void RecycleResource(std::unique_ptr<Resource> resource) {}

  // Implement impl::CommandBufferSequenceListener::CommandBufferFinished().
  // Checks whether it is safe to recycle any of |unused_resources_|.
  void CommandBufferFinished(uint64_t sequence_number) override;

  // Implement Owner::OnReceiveOwnable().  Call RecycleOwnable() immediately if
  // it is safe to do so.  Otherwise, adds the resource to a set of resources
  // to be recycled later; see CommandBufferFinished().
  void OnReceiveOwnable(std::unique_ptr<Resource> resource) override;

  uint64_t last_finished_sequence_number_ = 0;

  // We need to use an unordered_map instead of an unordered_set because you
  // can't modify elements of an unordered_set, which prevents us from
  // removing a unique_ptr.
  std::unordered_map<Resource*, std::unique_ptr<Resource>> unused_resources_;

  // TODO: use Escher* instead of EscherImpl*.
  // TODO: push this up into ResourceManager.
  impl::EscherImpl* const escher_;
};

}  // namespace escher
