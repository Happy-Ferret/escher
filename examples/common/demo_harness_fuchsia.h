// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <memory>

#include "application/lib/app/application_context.h"
#include "apps/modular/services/module/module.fidl.h"
#include "apps/modular/services/module/module_context.fidl.h"
#include "lib/escher/examples/common/demo_harness.h"
#include "lib/escher/examples/common/services/escher_demo.fidl.h"
#include "lib/fidl/cpp/bindings/binding_set.h"
#include "mtl/tasks/message_loop.h"

class DemoHarnessFuchsia : public DemoHarness,
                           public escher_demo::EscherDemo,
                           public modular::Module {
 public:
  DemoHarnessFuchsia(WindowParams window_params,
                     InstanceParams instance_params);

  // |DemoHarness|
  void Run(Demo* demo) override;

  // |EscherDemo|
  void HandleKeyPress(uint8_t key) override;
  void HandleTouchBegin(uint64_t touch_id, double xpos, double ypos) override;
  void HandleTouchContinue(uint64_t touch_id,
                           double xpos,
                           double ypos) override;
  void HandleTouchEnd(uint64_t touch_id, double xpos, double ypos) override;

  // |Module|
  void Initialize(
      fidl::InterfaceHandle<modular::ModuleContext> module_context,
      fidl::InterfaceHandle<app::ServiceProvider> incoming_services,
      fidl::InterfaceRequest<app::ServiceProvider> outgoing_services) override;

  // |Module|
  void Stop(const StopCallback& done) override;

 private:
  // Called by Init().
  void InitWindowSystem() override;
  void CreateWindowAndSurface(const WindowParams& window_params) override;

  // Called by Init() via CreateInstance().
  void AppendPlatformSpecificInstanceExtensionNames(
      InstanceParams* params) override;

  // Called by Shutdown().
  void ShutdownWindowSystem() override;

  void RenderFrameOrQuit();

  mtl::MessageLoop loop_;
  std::unique_ptr<app::ApplicationContext> application_context_;
  fidl::Binding<modular::Module> module_binding_;
  fidl::Binding<escher_demo::EscherDemo> escher_demo_binding_;
  std::unique_ptr<app::ServiceProviderImpl> outgoing_services_;
  fidl::InterfacePtr<modular::ModuleContext> module_context_;
};
