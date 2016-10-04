// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <vulkan/vulkan.hpp>

#include <vector>

#include "ftl/logging.h"

#if defined(ESCHER_VK_UTILS)
#error "vulkan_utils.h should not be included from header files"
#endif
#define ESCHER_VK_UTILS

// Log Vulkan error, if any.
#define ESCHER_LOG_VK_ERROR(ERR, MSG)                           \
  {                                                             \
    vk::Result error = ERR;                                     \
    const char* message = MSG;                                  \
    if (error != vk::Result::eSuccess) {                        \
      FTL_LOG(WARNING) << message << " : " << to_string(error); \
    }                                                           \
  }

namespace escher {
namespace impl {

// Panic if operation was unsuccessful.
template <typename T>
auto ESCHER_CHECKED_VK_RESULT(typename vk::ResultValue<T> result) -> T {
  FTL_CHECK(result.result == vk::Result::eSuccess);
  return result.value;
}

// Filter the |desired_formats| list to contain only those formats which support
// optimal tiling.
std::vector<vk::Format> GetSupportedDepthFormats(
    vk::PhysicalDevice device,
    std::vector<vk::Format> desired_formats);

// Pick the highest precision depth format that supports optimal tiling.
typedef vk::ResultValueType<vk::Format>::type FormatResult;
FormatResult GetSupportedDepthFormat(vk::PhysicalDevice device);

// Search through all memory types specified by |type_bits| and return the index
// of the first one that has all necessary flags.  Panic if nones is found.
uint32_t GetMemoryTypeIndex(vk::PhysicalDevice device,
                            uint32_t type_bits,
                            vk::MemoryPropertyFlags required_properties);

}  // namespace impl
}  // namespace escher
