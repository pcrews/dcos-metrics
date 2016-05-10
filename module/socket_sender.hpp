#pragma once

#include <boost/asio.hpp>
#include <set>

#include "params.hpp"

namespace metrics {

  /**
   * A SocketSender is the underlying implementation of getting data to an endpoint. It handles
   * periodically refreshing the destination endpoint for changes, along with passing any data to
   * the endpoint.
   */
  template <typename AsioProtocol>
  class SocketSender {
   public:
    /**
     * Creates an SocketSender which shares the provided io_service for async operations.
     * Additional arguments are exposed here to allow customization in unit tests.
     *
     * start() must be called before send()ing data, or else that data will be lost.
     */
    SocketSender(std::shared_ptr<boost::asio::io_service> io_service,
        const std::string& host,
        size_t port,
        size_t resolve_period_ms);

    virtual ~SocketSender();

    /**
     * Starts internal timers for refreshing the host.
     */
    void start();

    /**
     * Sends data to the current endpoint, or fails silently if the endpoint isn't available.
     * This call should only be performed from within the IO thread.
     */
    void send(const char* bytes, size_t size);

   protected:
    typedef typename AsioProtocol::resolver udp_resolver_t;

    /**
     * DNS lookup operation. Broken out for easier mocking in tests.
     */
    virtual typename AsioProtocol::resolver::iterator resolve(boost::system::error_code& ec);

    /**
     * Cancels running timers. Subclasses should call this in their destructor, to avoid the default
     * resolve() being called in the timespan between ~<Subclass>() and ~SocketSender().
     */
    void shutdown();

   private:
    typedef typename AsioProtocol::endpoint endpoint_t;

    void start_dest_resolve_timer();
    void dest_resolve_cb(boost::system::error_code ec);

    void shutdown_cb();

    const std::string send_host;
    const size_t send_port;
    const size_t resolve_period_ms;

    std::shared_ptr<boost::asio::io_service> io_service;
    boost::asio::deadline_timer resolve_timer;
    endpoint_t current_endpoint;
    std::multiset<boost::asio::ip::address> last_resolved_addresses;
    typename AsioProtocol::socket socket;
    size_t dropped_bytes;
  };

  template<>
  void SocketSender<boost::asio::ip::tcp>::send(const char* bytes, size_t size);
  template<>
  void SocketSender<boost::asio::ip::udp>::send(const char* bytes, size_t size);
}
