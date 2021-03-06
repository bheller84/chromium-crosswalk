// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/mojo/keep_alive_impl.h"

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "content/public/browser/notification_service.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/extensions_test.h"
#include "extensions/browser/mock_extension_system.h"
#include "extensions/browser/process_manager.h"
#include "extensions/browser/test_extensions_browser_client.h"
#include "extensions/common/extension_builder.h"

namespace extensions {

class KeepAliveTest : public ExtensionsTest {
 public:
  KeepAliveTest()
      : notification_service_(content::NotificationService::Create()) {}
  ~KeepAliveTest() override {}

  void SetUp() override {
    ExtensionsTest::SetUp();
    message_loop_.reset(new base::MessageLoop);
    browser_client_.reset(new TestExtensionsBrowserClient(browser_context()));
    browser_client_->set_extension_system_factory(&extension_system_factory_);
    ExtensionsBrowserClient::Set(browser_client_.get());
    extension_ =
        ExtensionBuilder()
            .SetManifest(
                 DictionaryBuilder()
                     .Set("name", "app")
                     .Set("version", "1")
                     .Set("manifest_version", 2)
                     .Set("app",
                          DictionaryBuilder().Set(
                              "background",
                              DictionaryBuilder().Set(
                                  "scripts",
                                  ListBuilder().Append("background.js")))))
            .SetID("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa")
            .Build();
  }

  void TearDown() override {
    message_loop_.reset();
    ExtensionsTest::TearDown();
  }

  void WaitUntilLazyKeepAliveChanges() {
    int initial_keep_alive_count = GetKeepAliveCount();
    while (GetKeepAliveCount() == initial_keep_alive_count) {
      base::RunLoop().RunUntilIdle();
    }
  }

  void CreateKeepAlive(mojo::InterfaceRequest<KeepAlive> request) {
    KeepAliveImpl::Create(browser_context(), extension_.get(), request.Pass());
  }

  const Extension* extension() { return extension_.get(); }

  int GetKeepAliveCount() {
    return ProcessManager::Get(browser_context())
        ->GetLazyKeepaliveCount(extension());
  }

 private:
  scoped_ptr<base::MessageLoop> message_loop_;
  MockExtensionSystemFactory<MockExtensionSystem> extension_system_factory_;
  scoped_ptr<content::NotificationService> notification_service_;
  scoped_refptr<const Extension> extension_;
  scoped_ptr<TestExtensionsBrowserClient> browser_client_;

  DISALLOW_COPY_AND_ASSIGN(KeepAliveTest);
};

TEST_F(KeepAliveTest, Basic) {
  mojo::InterfacePtr<KeepAlive> keep_alive;
  CreateKeepAlive(mojo::GetProxy(&keep_alive));
  EXPECT_EQ(1, GetKeepAliveCount());

  keep_alive.reset();
  WaitUntilLazyKeepAliveChanges();
  EXPECT_EQ(0, GetKeepAliveCount());
}

TEST_F(KeepAliveTest, TwoKeepAlives) {
  mojo::InterfacePtr<KeepAlive> keep_alive;
  CreateKeepAlive(mojo::GetProxy(&keep_alive));
  EXPECT_EQ(1, GetKeepAliveCount());

  mojo::InterfacePtr<KeepAlive> other_keep_alive;
  CreateKeepAlive(mojo::GetProxy(&other_keep_alive));
  EXPECT_EQ(2, GetKeepAliveCount());

  keep_alive.reset();
  WaitUntilLazyKeepAliveChanges();
  EXPECT_EQ(1, GetKeepAliveCount());

  other_keep_alive.reset();
  WaitUntilLazyKeepAliveChanges();
  EXPECT_EQ(0, GetKeepAliveCount());
}

}  // namespace extensions
