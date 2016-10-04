// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <cstdint>
#include <vulkan/vulkan.hpp>

namespace escher {

struct VulkanSwapchain {
  vk::SwapchainKHR swapchain;
  std::vector<vk::Image> images;
  std::vector<vk::ImageView> image_views;
  uint32_t width;
  uint32_t height;

  VulkanSwapchain(vk::SwapchainKHR swapchain_in,
                  std::vector<vk::Image> images_in,
                  std::vector<vk::ImageView> image_views_in,
                  uint32_t width_in,
                  uint32_t height_in)
      : swapchain(swapchain_in),
        images(std::move(images_in)),
        image_views(std::move(image_views_in)),
        width(width_in),
        height(height_in) {}

  VulkanSwapchain() : width(UINT32_MAX), height(UINT32_MAX) {}
};

}  // namespace escher
