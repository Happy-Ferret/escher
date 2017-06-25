// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "escher/examples/common/demo_harness_linux.h"

#include <GLFW/glfw3.h>

#include "escher/examples/common/demo.h"
#include "ftl/logging.h"

static DemoHarness* g_harness = nullptr;
static GLFWwindow* g_window;
// Current mouse position.
static double g_x_pos = 0.0;
static double g_y_pos = 0.0;
static bool g_touching = false;

// Helper for DemoHarness::InitWindowSystem().
static void DemoGlfwErrorCallback(int err_code, const char* err_desc) {
  FTL_LOG(WARNING) << "GLFW ERROR: " << err_code << " " << err_desc
                   << std::endl;
}

static void DemoGlfwKeyCallback(GLFWwindow* window,
                                int key,
                                int scancode,
                                int action,
                                int mods) {
  auto demo = g_harness->GetRunningDemo();
  if (!demo)
    return;

  // We only care about presses, not releases.
  if (action == GLFW_PRESS) {
    switch (key) {
      case GLFW_KEY_ESCAPE:
        demo->HandleKeyPress("ESCAPE");
        break;
      case GLFW_KEY_SPACE:
        demo->HandleKeyPress("SPACE");
        break;
      case GLFW_KEY_0:
      case GLFW_KEY_1:
      case GLFW_KEY_2:
      case GLFW_KEY_3:
      case GLFW_KEY_4:
      case GLFW_KEY_5:
      case GLFW_KEY_6:
      case GLFW_KEY_7:
      case GLFW_KEY_8:
      case GLFW_KEY_9: {
        char digit = '0' + (key - GLFW_KEY_0);
        demo->HandleKeyPress(std::string(1, digit));
        break;
      }
      case GLFW_KEY_A:
      case GLFW_KEY_B:
      case GLFW_KEY_C:
      case GLFW_KEY_D:
      case GLFW_KEY_E:
      case GLFW_KEY_F:
      case GLFW_KEY_G:
      case GLFW_KEY_H:
      case GLFW_KEY_I:
      case GLFW_KEY_J:
      case GLFW_KEY_K:
      case GLFW_KEY_L:
      case GLFW_KEY_M:
      case GLFW_KEY_N:
      case GLFW_KEY_O:
      case GLFW_KEY_P:
      case GLFW_KEY_Q:
      case GLFW_KEY_R:
      case GLFW_KEY_S:
      case GLFW_KEY_T:
      case GLFW_KEY_U:
      case GLFW_KEY_V:
      case GLFW_KEY_W:
      case GLFW_KEY_X:
      case GLFW_KEY_Y:
      case GLFW_KEY_Z: {
        char letter = 'A' + (key - GLFW_KEY_A);
        demo->HandleKeyPress(std::string(1, letter));
        break;
      }
      default:
        break;
    }
  }
}

static void DemoGlfwCursorPosCallback(GLFWwindow* window,
                                      double x_pos,
                                      double y_pos) {
  g_x_pos = x_pos;
  g_y_pos = y_pos;
  if (!g_touching) {
    // Simply remember the latest position, so that we know it when the mouse
    // button is pressed.
    return;
  }

  if (auto demo = g_harness->GetRunningDemo()) {
    demo->ContinueTouch(0, &g_x_pos, &g_y_pos, 1);
  }
}

static void DemoGlfwMouseButtonCallback(GLFWwindow* window,
                                        int button,
                                        int action,
                                        int mods) {
  if (button != GLFW_MOUSE_BUTTON_1) {
    // We only handle the primary mouse button.
    return;
  }

  if (auto demo = g_harness->GetRunningDemo()) {
    if (action == GLFW_PRESS) {
      FTL_CHECK(!g_touching);
      g_touching = true;
      demo->BeginTouch(0, g_x_pos, g_y_pos);
    } else {
      FTL_CHECK(g_touching);
      g_touching = false;
      demo->EndTouch(0, g_x_pos, g_y_pos);
    }
  }
}

// When running on Linux, New() instantiates a DemoHarnessLinux.
std::unique_ptr<DemoHarness> DemoHarness::New(
    DemoHarness::WindowParams window_params,
    DemoHarness::InstanceParams instance_params) {
  auto harness = new DemoHarnessLinux(window_params, instance_params);
  harness->Init();
  return std::unique_ptr<DemoHarness>(harness);
}

DemoHarnessLinux::DemoHarnessLinux(WindowParams window_params,
                                   InstanceParams instance_params)
    : DemoHarness(window_params, instance_params) {}

void DemoHarnessLinux::InitWindowSystem() {
  FTL_CHECK(!g_harness);
  g_harness = this;

  glfwSetErrorCallback(DemoGlfwErrorCallback);
  FTL_CHECK(glfwInit());
}

void DemoHarnessLinux::CreateWindowAndSurface(const WindowParams& params) {
  FTL_CHECK(!g_window);

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  GLFWmonitor* monitor =
      params.use_fullscreen ? glfwGetPrimaryMonitor() : nullptr;
  g_window = glfwCreateWindow(params.width, params.height,
                              params.window_name.c_str(), monitor, NULL);
  FTL_CHECK(g_window);

  VkSurfaceKHR surface;
  VkResult err = glfwCreateWindowSurface(instance(), g_window, NULL, &surface);
  FTL_CHECK(!err);
  set_surface(surface);

  glfwSetKeyCallback(g_window, DemoGlfwKeyCallback);
  glfwSetCursorPosCallback(g_window, DemoGlfwCursorPosCallback);
  glfwSetMouseButtonCallback(g_window, DemoGlfwMouseButtonCallback);
}

void DemoHarnessLinux::AppendPlatformSpecificInstanceExtensionNames(
    InstanceParams* params) {
  // Get names of extensions required by GLFW.
  uint32_t extensions_count;
  const char** extensions =
      glfwGetRequiredInstanceExtensions(&extensions_count);
  for (uint32_t i = 0; i < extensions_count; ++i) {
    params->extension_names.emplace_back(std::string(extensions[i]));
  }
}

void DemoHarnessLinux::ShutdownWindowSystem() {
  FTL_CHECK(g_harness);
  g_harness = nullptr;
  g_window = nullptr;
  glfwTerminate();
}

void DemoHarnessLinux::Run(Demo* demo) {
  FTL_CHECK(!demo_);
  demo_ = demo;

  while (!this->ShouldQuit()) {
    demo->DrawFrame();
    glfwPollEvents();
  }
  device().waitIdle();
  glfwSetWindowShouldClose(g_window, GLFW_TRUE);

  demo_ = nullptr;
}
