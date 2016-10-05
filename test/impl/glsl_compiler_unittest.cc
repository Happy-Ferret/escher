// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "escher/impl/glsl_compiler.h"
#include "ftl/logging.h"
#include "gtest/gtest.h"

namespace escher {
namespace impl {
namespace {

constexpr char vertex_src[] = R"GLSL(
  #version 400
  #extension GL_ARB_separate_shader_objects : enable
  #extension GL_ARB_shading_language_420pack : enable
  layout (location = 0) in vec4 pos;
  layout (location = 1) in vec2 attr;
  layout (location = 0) out vec2 texcoord;
  out gl_PerVertex {
    vec4 gl_Position;
  };
  void main() {
    texcoord = attr;
    gl_Position = pos;
  }
  )GLSL";

constexpr char fragment_src[] = R"GLSL(
  #version 400
  #extension GL_ARB_separate_shader_objects : enable
  #extension GL_ARB_shading_language_420pack : enable
  layout (binding = 0) uniform sampler2D tex;
  layout (location = 0) in vec2 texcoord;
  layout (location = 0) out vec4 uFragColor;
  void main() {
    uFragColor = texture(tex, texcoord);
  }
  )GLSL";

TEST(GlslCompiler, CompileVertexShader) {
  GlslToSpirvCompiler compiler;
  std::vector<std::string> src = {{vertex_src}};
  SpirvData spirv =
      compiler.Compile(vk::ShaderStageFlagBits::eVertex, src, "", "main").get();
  EXPECT_GE(spirv.size(), 0U);
}

TEST(GlslCompiler, CompileFragmentShader) {
  GlslToSpirvCompiler compiler;
  std::vector<std::string> src = {{fragment_src}};
  SpirvData spirv =
      compiler.Compile(vk::ShaderStageFlagBits::eFragment, src, "", "").get();
  EXPECT_GE(spirv.size(), 0U);
}

TEST(GlslCompiler, CompileVertexShaderAsFragmentShader) {
  GlslToSpirvCompiler compiler;
  std::vector<std::string> src = {{vertex_src}};
  FTL_LOG(INFO) << "NOTE: the compiler errors below are expected.";
  SpirvData spirv =
      compiler.Compile(vk::ShaderStageFlagBits::eFragment, src, "", "").get();
  EXPECT_EQ(spirv.size(), 0U);
}

}  // namespace
}  // namespace impl
}  // namespace escher
