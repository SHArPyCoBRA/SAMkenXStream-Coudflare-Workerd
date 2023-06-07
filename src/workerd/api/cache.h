// Copyright (c) 2017-2022 Cloudflare, Inc.
// Licensed under the Apache 2.0 license found in the LICENSE file or at:
//     https://opensource.org/licenses/Apache-2.0

#pragma once

#include <workerd/jsg/jsg.h>
#include "http.h"

namespace workerd::api {

// =======================================================================================
// Cache

struct CacheQueryOptions {
  jsg::Optional<bool> ignoreMethod;
  // By default, Cache.match() and Cache.delete() will return undefined/false if passed a non-GET
  // request. Setting `ignoreMethod` to true disables this behavior; Cache.match() and
  // Cache.delete() will treat any request as a GET request.

  jsg::WontImplement ignoreSearch;
  // Our cache does not support matching without query parameters at match time. Users can still
  // remove query parameters before put()ing the Request/Response pair, if they wish.

  jsg::WontImplement ignoreVary;
  // Historically, Cloudflare has not supported the Vary header because it's easy to blow up your
  // cache keys. Customers can now implement this with workers by modifying cache keys as they see
  // fit based on any arbitary parameter (User-Agent, Content-Encoding, etc.).

  jsg::WontImplement cacheName;
  // Only used in CacheStorage::match(), which we won't implement.

  JSG_STRUCT(ignoreMethod, ignoreSearch, ignoreVary, cacheName);
};

class Cache: public jsg::Object {
public:
  explicit Cache(kj::Maybe<kj::String> cacheName);

  jsg::Unimplemented add(Request::Info request);
  jsg::Unimplemented addAll(kj::Array<Request::Info> requests);

  jsg::Promise<jsg::Optional<jsg::Ref<Response>>> match(
      jsg::Lock& js, Request::Info request, jsg::Optional<CacheQueryOptions> options);

  jsg::Promise<void> put(jsg::Lock& js, Request::Info request, jsg::Ref<Response> response,
      CompatibilityFlags::Reader flags);

  jsg::Promise<bool> delete_(
      jsg::Lock& js, Request::Info request, jsg::Optional<CacheQueryOptions> options);

  jsg::WontImplement matchAll(
      jsg::Optional<Request::Info>, jsg::Optional<CacheQueryOptions>) { return {}; }
  // Our cache does not support one-to-many matching, so this is not possible to implement.

  jsg::WontImplement keys(
      jsg::Optional<Request::Info>, jsg::Optional<CacheQueryOptions>) { return {}; }
  // Our cache does not support cache item enumeration, so this is not possible to implement.

  JSG_RESOURCE_TYPE(Cache) {
    JSG_METHOD(add);
    JSG_METHOD(addAll);
    JSG_METHOD_NAMED(delete, delete_);
    JSG_METHOD(match);
    JSG_METHOD(put);
    JSG_METHOD(matchAll);
    JSG_METHOD(keys);

    JSG_TS_OVERRIDE({
      delete(request: RequestInfo, options?: CacheQueryOptions): Promise<boolean>;
      match(request: RequestInfo, options?: CacheQueryOptions): Promise<Response | undefined>;
      put(request: RequestInfo, response: Response): Promise<void>;
    });
    // Use RequestInfo type alias to allow `URL`s as cache keys
  }

private:
  kj::Maybe<kj::String> cacheName;

  kj::Own<kj::HttpClient> getHttpClient(IoContext& context,
      kj::Maybe<kj::String> cfBlobJson, kj::StringPtr operationName);
};

// =======================================================================================
// CacheStorage

class CacheStorage: public jsg::Object {
public:
  CacheStorage();

  jsg::Promise<jsg::Ref<Cache>> open(jsg::Lock& js, kj::String cacheName);

  jsg::Ref<Cache> getDefault() { return default_.addRef(); }

  jsg::WontImplement match(Request::Info, jsg::Optional<CacheQueryOptions>) { return {}; }
  jsg::WontImplement has(kj::String) { return {}; }
  jsg::WontImplement delete_(kj::String) { return {}; }
  jsg::WontImplement keys() { return {}; }
  // Our cache does not support namespace enumeration, so none of these are possible to implement.

  JSG_RESOURCE_TYPE(CacheStorage) {
    JSG_METHOD(open);
    JSG_METHOD_NAMED(delete, delete_);
    JSG_METHOD(match);
    JSG_METHOD(has);
    JSG_METHOD(keys);

    JSG_READONLY_INSTANCE_PROPERTY(default, getDefault);
  }

private:
  jsg::Ref<Cache> default_;
};

#define EW_CACHE_ISOLATE_TYPES \
  api::CacheStorage,           \
  api::Cache,                  \
  api::CacheQueryOptions

}  // namespace workerd::api
