#pragma once

#include "envoy/upstream/cluster_manager.h"

#include "extensions/filters/network/common/redis/codec_impl.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace Common {
namespace Redis {
namespace Client {

/**
 * A handle to an outbound request.
 */
class PoolRequest {
public:
  virtual ~PoolRequest() {}

  /**
   * Cancel the request. No further request callbacks will be called.
   */
  virtual void cancel() PURE;
};

/**
 * Outbound request callbacks.
 */
class PoolCallbacks {
public:
  virtual ~PoolCallbacks() {}

  /**
   * Called when a pipelined response is received.
   * @param value supplies the response which is now owned by the callee.
   */
  virtual void onResponse(RespValuePtr&& value) PURE;

  /**
   * Called when a network/protocol error occurs and there is no response.
   */
  virtual void onFailure() PURE;

  /**
   * Called when a MOVED or ASK redirection error is received, and the request must be retried.
   * @param value supplies the MOVED error response
   * @return bool true if the request is successfully redirected, false otherwise
   */
  virtual bool onRedirection(const Common::Redis::RespValue& value) PURE;
};

/**
 * A single redis client connection.
 */
class Client : public Event::DeferredDeletable {
public:
  virtual ~Client() {}

  /**
   * Adds network connection callbacks to the underlying network connection.
   */
  virtual void addConnectionCallbacks(Network::ConnectionCallbacks& callbacks) PURE;

  /**
   * Closes the underlying network connection.
   */
  virtual void close() PURE;

  /**
   * Make a pipelined request to the remote redis server.
   * @param request supplies the RESP request to make.
   * @param callbacks supplies the request callbacks.
   * @return PoolRequest* a handle to the active request or nullptr if the request could not be made
   *         for some reason.
   */
  virtual PoolRequest* makeRequest(const RespValue& request, PoolCallbacks& callbacks) PURE;
};

typedef std::unique_ptr<Client> ClientPtr;

/**
 * Configuration for a redis connection pool.
 */
class Config {
public:
  virtual ~Config() {}

  /**
   * @return std::chrono::milliseconds the timeout for an individual redis operation. Currently,
   *         all operations use the same timeout.
   */
  virtual std::chrono::milliseconds opTimeout() const PURE;

  /**
   * @return bool disable outlier events even if the cluster has it enabled. This is used by the
   * healthchecker's connection pool to avoid double counting active healthcheck operations as
   * passive healthcheck operations.
   */
  virtual bool disableOutlierEvents() const PURE;

  /**
   * @return when enabled, a hash tagging function will be used to guarantee that keys with the
   * same hash tag will be forwarded to the same upstream.
   */
  virtual bool enableHashtagging() const PURE;

  /**
   * @return when enabled, moved/ask redirection errors from upstream redis servers will be
   * processed.
   */
  virtual bool enableRedirection() const PURE;
};

/**
 * A factory for individual redis client connections.
 */
class ClientFactory {
public:
  virtual ~ClientFactory() {}

  /**
   * Create a client given an upstream host.
   * @param host supplies the upstream host.
   * @param dispatcher supplies the owning thread's dispatcher.
   * @param config supplies the connection pool configuration.
   * @return ClientPtr a new connection pool client.
   */
  virtual ClientPtr create(Upstream::HostConstSharedPtr host, Event::Dispatcher& dispatcher,
                           const Config& config) PURE;
};

} // namespace Client
} // namespace Redis
} // namespace Common
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
