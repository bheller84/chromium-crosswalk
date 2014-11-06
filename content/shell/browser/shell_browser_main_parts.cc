// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/browser/shell_browser_main_parts.h"

#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/devtools_http_handler.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/main_function_params.h"
#include "content/public/common/url_constants.h"
#include "content/shell/browser/layout_test/layout_test_browser_context.h"
#include "content/shell/browser/shell.h"
#include "content/shell/browser/shell_browser_context.h"
#include "content/shell/browser/shell_devtools_manager_delegate.h"
#include "content/shell/browser/shell_net_log.h"
#include "content/shell/common/shell_switches.h"
#include "net/base/filename_util.h"
#include "net/base/net_module.h"
#include "net/grit/net_resources.h"
#include "ui/base/resource/resource_bundle.h"
#include "url/gurl.h"

#if defined(OS_ANDROID)
#include "components/crash/browser/crash_dump_manager_android.h"
#include "net/android/network_change_notifier_factory_android.h"
#include "net/base/network_change_notifier.h"
#endif

#if defined(USE_AURA) && defined(USE_X11)
#include "ui/events/devices/x11/touch_factory_x11.h"
#endif
#if !defined(OS_CHROMEOS) && defined(USE_AURA) && defined(OS_LINUX)
#include "ui/base/ime/input_method_initializer.h"
#endif

namespace content {

namespace {

GURL GetStartupURL() {
  CommandLine* command_line = CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kContentBrowserTest))
    return GURL();
  const CommandLine::StringVector& args = command_line->GetArgs();

#if defined(OS_ANDROID)
  // Delay renderer creation on Android until surface is ready.
  return GURL();
#endif

  if (args.empty())
    return GURL("http://www.google.com/");

  GURL url(args[0]);
  if (url.is_valid() && url.has_scheme())
    return url;

  return net::FilePathToFileURL(base::FilePath(args[0]));
}

base::StringPiece PlatformResourceProvider(int key) {
  if (key == IDR_DIR_HEADER_HTML) {
    base::StringPiece html_data =
        ui::ResourceBundle::GetSharedInstance().GetRawDataResource(
            IDR_DIR_HEADER_HTML);
    return html_data;
  }
  return base::StringPiece();
}

}  // namespace

ShellBrowserMainParts::ShellBrowserMainParts(
    const MainFunctionParams& parameters)
    : parameters_(parameters),
      run_message_loop_(true),
      devtools_http_handler_(nullptr) {
}

ShellBrowserMainParts::~ShellBrowserMainParts() {
  if (devtools_http_handler_) {
    // Note that Stop destroys devtools_http_handler_.
    devtools_http_handler_->Stop();
  }
}

#if !defined(OS_MACOSX)
void ShellBrowserMainParts::PreMainMessageLoopStart() {
#if defined(USE_AURA) && defined(USE_X11)
  ui::TouchFactory::SetTouchDeviceListFromCommandLine();
#endif
}
#endif

void ShellBrowserMainParts::PostMainMessageLoopStart() {
#if defined(OS_ANDROID)
  base::MessageLoopForUI::current()->Start();
#endif
}

void ShellBrowserMainParts::PreEarlyInitialization() {
#if !defined(OS_CHROMEOS) && defined(USE_AURA) && defined(OS_LINUX)
  ui::InitializeInputMethodForTesting();
#endif
#if defined(OS_ANDROID)
  net::NetworkChangeNotifier::SetFactory(
      new net::NetworkChangeNotifierFactoryAndroid());
#endif
}

void ShellBrowserMainParts::InitializeBrowserContexts() {
  set_browser_context(new ShellBrowserContext(false, net_log_.get()));
  set_off_the_record_browser_context(
      new ShellBrowserContext(true, net_log_.get()));
}

void ShellBrowserMainParts::InitializeMessageLoopContext() {
  Shell::CreateNewWindow(browser_context_.get(),
                         GetStartupURL(),
                         NULL,
                         MSG_ROUTING_NONE,
                         gfx::Size());
}

void ShellBrowserMainParts::PreMainMessageLoopRun() {
#if defined(OS_ANDROID)
  if (CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableCrashReporter)) {
    base::FilePath crash_dumps_dir =
        CommandLine::ForCurrentProcess()->GetSwitchValuePath(
            switches::kCrashDumpsDir);
    crash_dump_manager_.reset(new breakpad::CrashDumpManager(crash_dumps_dir));
  }
#endif

  net_log_.reset(new ShellNetLog("content_shell"));
  InitializeBrowserContexts();
  Shell::Initialize();
  net::NetModule::SetResourceProvider(PlatformResourceProvider);

  // CreateHttpHandler retains ownership over DevToolsHttpHandler.
  devtools_http_handler_ =
      ShellDevToolsManagerDelegate::CreateHttpHandler(browser_context_.get());

  InitializeMessageLoopContext();

  if (parameters_.ui_task) {
    parameters_.ui_task->Run();
    delete parameters_.ui_task;
    run_message_loop_ = false;
  }
}

bool ShellBrowserMainParts::MainMessageLoopRun(int* result_code)  {
  return !run_message_loop_;
}

void ShellBrowserMainParts::PostMainMessageLoopRun() {
  if (devtools_http_handler_) {
    // Note that Stop destroys devtools_http_handler_.
    devtools_http_handler_->Stop();
    devtools_http_handler_ = nullptr;
  }
  browser_context_.reset();
  off_the_record_browser_context_.reset();
}

}  // namespace
