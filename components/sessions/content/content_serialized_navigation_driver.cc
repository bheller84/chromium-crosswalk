// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sessions/content/content_serialized_navigation_driver.h"

#include "base/memory/singleton.h"
#include "components/sessions/serialized_navigation_entry.h"
#include "content/public/common/page_state.h"
#include "content/public/common/referrer.h"

namespace sessions {

// static
SerializedNavigationDriver* SerializedNavigationDriver::Get() {
  return ContentSerializedNavigationDriver::GetInstance();
}

// static
ContentSerializedNavigationDriver*
ContentSerializedNavigationDriver::GetInstance() {
  return Singleton<ContentSerializedNavigationDriver,
      LeakySingletonTraits<ContentSerializedNavigationDriver>>::get();
}

ContentSerializedNavigationDriver::ContentSerializedNavigationDriver() {
}

ContentSerializedNavigationDriver::~ContentSerializedNavigationDriver() {
}

int ContentSerializedNavigationDriver::GetDefaultReferrerPolicy() const {
  return blink::WebReferrerPolicyDefault;
}

std::string
ContentSerializedNavigationDriver::GetSanitizedPageStateForPickle(
    const SerializedNavigationEntry* navigation) const {
  if (!navigation->has_post_data_) {
    return navigation->encoded_page_state_;
  }
  content::PageState page_state =
      content::PageState::CreateFromEncodedData(
          navigation->encoded_page_state_);
  return page_state.RemovePasswordData().ToEncodedData();
}

void ContentSerializedNavigationDriver::Sanitize(
    SerializedNavigationEntry* navigation) const {
  content::Referrer old_referrer(
      navigation->referrer_url_,
      static_cast<blink::WebReferrerPolicy>(navigation->referrer_policy_));
  content::Referrer new_referrer =
      content::Referrer::SanitizeForRequest(navigation->virtual_url_,
                                            old_referrer);

  // No need to compare the policy, as it doesn't change during
  // sanitization. If there has been a change, the referrer needs to be
  // stripped from the page state as well.
  if (navigation->referrer_url_ != new_referrer.url) {
    navigation->referrer_url_ = GURL();
    navigation->referrer_policy_ = GetDefaultReferrerPolicy();
    navigation->encoded_page_state_ =
        content::PageState::CreateFromEncodedData(
            navigation->encoded_page_state_)
            .RemoveReferrer()
            .ToEncodedData();
  }
}

}  // namespace sessions
