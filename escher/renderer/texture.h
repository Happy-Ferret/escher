// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "escher/forward_declarations.h"
#include "escher/impl/resource.h"

namespace escher {

class Texture : public impl::Resource {
 public:
  Texture(ImagePtr image, vk::Device device, vk::Filter filter);
  Texture(ImagePtr image,
          vk::Device device,
          vk::Filter filter,
          vk::ImageAspectFlags aspect_mask,
          bool use_unnormalized_coordinates = false);
  ~Texture() override;

  const ImagePtr& image() const { return image_; }
  vk::ImageView image_view() const { return image_view_; }
  vk::Sampler sampler() const { return sampler_; }
  uint32_t width() const { return width_; }
  uint32_t height() const { return height_; }

 private:
  void Init(vk::Filter filter,
            vk::ImageAspectFlags aspect_mask,
            bool use_unnormalized_coordinates);

  ImagePtr image_;
  vk::Device device_;
  vk::ImageView image_view_;
  vk::Sampler sampler_;
  uint32_t width_;
  uint32_t height_;

  FTL_DISALLOW_COPY_AND_ASSIGN(Texture);
};

typedef ftl::RefPtr<Texture> TexturePtr;

}  // namespace escher
