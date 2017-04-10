// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "escher/examples/common/demo_harness_fuchsia.h"

#include "escher/examples/common/demo.h"

// When running on Fuchsia, New() instantiates a DemoHarnessFuchsia.
std::unique_ptr<DemoHarness> DemoHarness::New(
    DemoHarness::WindowParams window_params,
    DemoHarness::InstanceParams instance_params) {
  auto harness = new DemoHarnessFuchsia(window_params, instance_params);
  harness->Init();
  return std::unique_ptr<DemoHarness>(harness);
}

DemoHarnessFuchsia::DemoHarnessFuchsia(WindowParams window_params,
                                       InstanceParams instance_params)
    : DemoHarness(window_params, instance_params),
      application_context_(app::ApplicationContext::CreateFromStartupInfo()),
      module_binding_(this),
      escher_demo_binding_(this) {
  application_context_->outgoing_services()->AddService<modular::Module>(
      [this](fidl::InterfaceRequest<modular::Module> request) {
        FTL_DCHECK(!module_binding_.is_bound());
        module_binding_.Bind(std::move(request));
      });
}

void DemoHarnessFuchsia::InitWindowSystem() {}

void DemoHarnessFuchsia::CreateWindowAndSurface(const WindowParams& params) {
  VkMagmaSurfaceCreateInfoKHR create_info = {
      .sType = VK_STRUCTURE_TYPE_MAGMA_SURFACE_CREATE_INFO_KHR,
      .pNext = nullptr,
  };
  VkSurfaceKHR surface;
  VkResult err =
      vkCreateMagmaSurfaceKHR(instance(), &create_info, nullptr, &surface);
  FTL_CHECK(!err);
  set_surface(surface);
}

void DemoHarnessFuchsia::AppendPlatformSpecificInstanceExtensionNames(
    InstanceParams* params) {
  params->extension_names.emplace_back(
      std::string(VK_KHR_SURFACE_EXTENSION_NAME));
  params->extension_names.emplace_back(
      std::string(VK_KHR_MAGMA_SURFACE_EXTENSION_NAME));
}

void DemoHarnessFuchsia::ShutdownWindowSystem() {}

void DemoHarnessFuchsia::Run(Demo* demo) {
  FTL_CHECK(!demo_);
  demo_ = demo;
  loop_.task_runner()->PostTask([this] { this->RenderFrameOrQuit(); });
  loop_.Run();
}

void DemoHarnessFuchsia::RenderFrameOrQuit() {
  FTL_CHECK(demo_);  // Must be running.
  if (ShouldQuit()) {
    loop_.QuitNow();
    device().waitIdle();
  } else {
    demo_->DrawFrame();
    loop_.task_runner()->PostDelayedTask([this] { this->RenderFrameOrQuit(); },
                                         ftl::TimeDelta::FromMilliseconds(1));
  }
}

void DemoHarnessFuchsia::HandleKeyPress(uint8_t key) {
  demo_->HandleKeyPress(std::string(1, static_cast<char>(key)));
}

void DemoHarnessFuchsia::HandleTouchBegin(uint64_t touch_id,
                                          double xpos,
                                          double ypos) {
  demo_->BeginTouch(touch_id, xpos, ypos);
}

void DemoHarnessFuchsia::HandleTouchContinue(uint64_t touch_id,
                                             double xpos,
                                             double ypos) {
  demo_->ContinueTouch(touch_id, &xpos, &ypos, 1);
}

void DemoHarnessFuchsia::HandleTouchEnd(uint64_t touch_id,
                                        double xpos,
                                        double ypos) {
  demo_->EndTouch(touch_id, xpos, ypos);
}

// |Module|
void DemoHarnessFuchsia::Initialize(
    fidl::InterfaceHandle<modular::ModuleContext> module_context,
    fidl::InterfaceHandle<modular::Link> link,
    fidl::InterfaceHandle<app::ServiceProvider> incoming_services,
    fidl::InterfaceRequest<app::ServiceProvider> outgoing_services) {
  module_context_.Bind(std::move(module_context));

  // Provide EscherDemo service only to the Module that started us.
  outgoing_services_ =
      std::make_unique<app::ServiceProviderImpl>(std::move(outgoing_services));
  outgoing_services_->AddService<escher_demo::EscherDemo>(
      [this](fidl::InterfaceRequest<escher_demo::EscherDemo> request) {
        FTL_DCHECK(!escher_demo_binding_.is_bound());
        escher_demo_binding_.Bind(std::move(request));
      });
}

// |Module|
void DemoHarnessFuchsia::Stop(const StopCallback& done) {
  module_context_.reset();
  done();
}
