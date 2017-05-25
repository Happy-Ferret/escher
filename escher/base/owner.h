// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "escher/base/ownable.h"
#include "ftl/memory/ref_counted.h"

namespace escher {

// Subclasses of Owner manage the lifecycle of Ownable objects.  When the
// ref-count of an Ownable reaches zero, Owner::ReceiveOwnable() is invoked;
// the Owner is then responsible for deciding whether to destroy it, recycle it,
// etc.  The Owner may only own Ownables that are parameterized with the same
// TypeInfoT type.
template <typename OwnableT, typename TypeInfoT>
class Owner {
 public:
  virtual ~Owner() { FTL_DCHECK(ownable_count_ == 0); }

  // Return the number of Ownables currently owned by this owner.  Must be
  // zero when the Owner is destroyed.
  uint32_t ownable_count() const { return ownable_count_; }

 protected:
  // Allow subclasses of Owner to take ownership of |unowned_ownable|, which
  // must not already have an owner.
  void BecomeOwnerOf(OwnableT* ownable) {
    FTL_DCHECK(ownable && ownable->owner() == nullptr);
    ownable->set_owner(this);
    IncrementOwnableCount();
  }

  // Allow subclasses of Owner to relinquish ownership of |ownable|; afterward,
  // it is safe for |ownable| to be destroyed.  This must not be called if
  // this Owner does not own |ownable|.
  void RelinquishOwnershipOf(OwnableT* ownable) {
    FTL_DCHECK(ownable && ownable->owner() == this);
    ownable->set_owner(nullptr);
    DecrementOwnableCount();
  }

 private:
  friend class Ownable<OwnableT, TypeInfoT>;

  // Called by Ownable::OnZeroRefCount().  This owner is now responsible for
  // the lifecycle of the dereferenced Ownable.
  void ReceiveOwnable(std::unique_ptr<OwnableT> unreffed) {
    FTL_DCHECK(unreffed && unreffed->owner() == this);
    OnReceiveOwnable(std::move(unreffed));
  }

  // Called by ReceiveOwnable() to allow subclasses to specify what should
  // happen to the unreffed Ownable.  This is a separate function to guarantee
  // that the checks in ReceiveOwnable() always take place.
  virtual void OnReceiveOwnable(std::unique_ptr<OwnableT> unreffed) = 0;

  // Ownables hold a raw pointer to their owner.  This ref-count allows us to
  // detect programming errors that cause an Ownable to outlive its Owner.
  void IncrementOwnableCount() { ++ownable_count_; }
  void DecrementOwnableCount() { --ownable_count_; }
  uint32_t ownable_count_ = 0;
};

}  // namespace escher
