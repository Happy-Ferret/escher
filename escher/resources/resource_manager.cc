// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "escher/resources/resource_manager.h"

#include "escher/escher.h"

namespace escher {

// TODO: DemoHarness::SwapchainImageOwner is currently instantiated before
// an Escher exists.  Fix this, then assert that Escher is non-null here.
ResourceManager::ResourceManager(Escher* escher)
    : escher_(escher),
      vulkan_context_(escher ? escher->vulkan_context() : VulkanContext()) {}

}  // namespace escher
