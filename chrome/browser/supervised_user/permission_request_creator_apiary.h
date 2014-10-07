// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SUPERVISED_USER_PERMISSION_REQUEST_CREATOR_APIARY_H_
#define CHROME_BROWSER_SUPERVISED_USER_PERMISSION_REQUEST_CREATOR_APIARY_H_

#include <string>

#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "chrome/browser/supervised_user/permission_request_creator.h"
#include "google_apis/gaia/oauth2_token_service.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

class Profile;
class SupervisedUserSigninManagerWrapper;

namespace base {
class Time;
}

namespace net {
class URLFetcher;
class URLRequestContextGetter;
}

class PermissionRequestCreatorApiary : public PermissionRequestCreator,
                                       public OAuth2TokenService::Consumer,
                                       public net::URLFetcherDelegate {
 public:
  PermissionRequestCreatorApiary(
      OAuth2TokenService* oauth2_token_service,
      scoped_ptr<SupervisedUserSigninManagerWrapper> signin_wrapper,
      net::URLRequestContextGetter* context,
      const GURL& apiary_url);
  virtual ~PermissionRequestCreatorApiary();

  static scoped_ptr<PermissionRequestCreator> CreateWithProfile(
      Profile* profile, const GURL& apiary_url);

  // PermissionRequestCreator implementation:
  virtual bool IsEnabled() const override;
  virtual void CreatePermissionRequest(
      const GURL& url_requested,
      const SuccessCallback& callback) override;

  void set_url_fetcher_id_for_testing(int id) { url_fetcher_id_ = id; }

 private:
  struct Request;
  typedef ScopedVector<Request>::iterator RequestIterator;

  // OAuth2TokenService::Consumer implementation:
  virtual void OnGetTokenSuccess(const OAuth2TokenService::Request* request,
                                 const std::string& access_token,
                                 const base::Time& expiration_time) override;
  virtual void OnGetTokenFailure(const OAuth2TokenService::Request* request,
                                 const GoogleServiceAuthError& error) override;

  // net::URLFetcherDelegate implementation.
  virtual void OnURLFetchComplete(const net::URLFetcher* source) override;

  std::string GetApiScopeToUse() const;

  // Requests an access token, which is the first thing we need. This is where
  // we restart when the returned access token has expired.
  void StartFetching(Request* request);

  void DispatchNetworkError(RequestIterator it, int error_code);
  void DispatchGoogleServiceAuthError(RequestIterator it,
                                      const GoogleServiceAuthError& error);

  OAuth2TokenService* oauth2_token_service_;
  scoped_ptr<SupervisedUserSigninManagerWrapper> signin_wrapper_;
  net::URLRequestContextGetter* context_;
  GURL apiary_url_;
  int url_fetcher_id_;

  ScopedVector<Request> requests_;
};

#endif  // CHROME_BROWSER_SUPERVISED_USER_PERMISSION_REQUEST_CREATOR_APIARY_H_
