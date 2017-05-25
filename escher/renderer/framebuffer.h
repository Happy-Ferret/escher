// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <vulkan/vulkan.hpp>

#include "escher/forward_declarations.h"
#include "escher/resources/resource.h"

namespace escher {

class Framebuffer : public Resource2 {
 public:
  static const ResourceTypeInfo kTypeInfo;
  const ResourceTypeInfo& type_info() const override { return kTypeInfo; }

  Framebuffer(impl::EscherImpl* escher,
              uint32_t width,
              uint32_t height,
              std::vector<ImagePtr> images,
              vk::RenderPass render_pass);
  ~Framebuffer() override;

  // TODO: make private... client shouldn't need access to this.
  vk::Framebuffer get() { return framebuffer_; }

  uint32_t width() const { return width_; }
  uint32_t height() const { return height_; }

  const ImagePtr& get_image(uint32_t index) const { return images_.at(index); }

 private:
  vk::Framebuffer framebuffer_;
  std::vector<vk::ImageView> image_views_;

  uint32_t width_;
  uint32_t height_;

  // These images are not used directly; they just ensure that the images are
  // not destroyed before the Framebuffer is.
  std::vector<ImagePtr> images_;

  FTL_DISALLOW_COPY_AND_ASSIGN(Framebuffer);
};

typedef ftl::RefPtr<Framebuffer> FramebufferPtr;

}  // namespace escher
