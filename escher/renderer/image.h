// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include "escher/forward_declarations.h"
#include "escher/impl/resource.h"

namespace escher {

// Full description of the size and layout of an Image.
struct ImageInfo {
  vk::Format format;
  uint32_t width;
  uint32_t height;
  // TODO: add more fields, like number of mip-levels.

  // Convenient way to populate an ImageInfo.
  ImageInfo(const vk::ImageCreateInfo& create_info);
  ImageInfo(vk::Format format, uint32_t width, uint32_t height);
};

// Every Image has an owner, who is responsible for cleaning up the Image's
// underlying resources when it is destroyed.  The ImageOwner must outlive all
// of its owned Images.
class ImageOwner;

// Encapsulates a vk::Image.  Lifecycle is managed by an ImageOwner.
class Image : public impl::Resource {
 public:
  // Returns image_ and mem_ to the owner.
  ~Image() override;

  vk::Image get() const { return image_; }
  vk::Format format() const { return info_.format; }
  uint32_t width() const { return info_.width; }
  uint32_t height() const { return info_.height; }

  bool has_depth() const { return has_depth_; }
  bool has_stencil() const { return has_stencil_; }

 private:
  // Only subclasses of ImageOwner are allowed to instantiate new Images.
  friend class ImageOwner;
  Image(ImageInfo info,
        vk::Image image,
        impl::GpuMemPtr mem,
        ImageOwner* owner);

  const ImageInfo info_;
  const vk::Image image_;
  impl::GpuMemPtr mem_;
  ImageOwner* const owner_;

  bool has_depth_;
  bool has_stencil_;

  FTL_DISALLOW_COPY_AND_ASSIGN(Image);
};

typedef ftl::RefPtr<Image> ImagePtr;

}  // namespace escher
