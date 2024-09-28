#include "server.h"

#include <spdlog/spdlog.h>

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

Server::Server(boost::asio::io_context& ioc) : ioc_(ioc) {}

template <class Body, class Allocator>
http::message_generator handle_request(
    http::request<Body, http::basic_fields<Allocator>>&& req) {
  // Returns a bad request response
  auto const bad_request = [&req](beast::string_view why) {
    http::response<http::string_body> res{
        http::status::bad_request, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = std::string(why);
    res.prepare_payload();
    spdlog::info("Response: {}", res.result_int());
    return res;
  };
  return bad_request("Not implemented yet");
}

asio::awaitable<void> do_session(beast::tcp_stream stream) {
  // This buffer is required to persist across reads
  beast::flat_buffer buffer;

  for (;;) {
    // Set the timeout.
    stream.expires_after(std::chrono::seconds(30));

    // Read a request
    http::request<http::string_body> req;
    co_await http::async_read(stream, buffer, req);
    spdlog::info("Request: {}", req.method_string().data());
    // Handle the request
    http::message_generator msg = handle_request(std::move(req));

    // Determine if we should close the connection
    bool keep_alive = msg.keep_alive();

    // Send the response
    co_await beast::async_write(stream, std::move(msg));

    if (!keep_alive) {
      // This means we should close the connection, usually because
      // the response indicated the "Connection: close" semantic.
      break;
    }
  }

  // Send a TCP shutdown
  spdlog::info("Shutting down session");
  stream.socket().shutdown(asio::ip::tcp::socket::shutdown_send);
  spdlog::info("Shut down session");

  // At this point the connection is closed gracefully. We ignore the error
  // because the client might have dropped the connection already.
}

// Accepts incoming connections and launches the sessions
asio::awaitable<void> do_listen(asio::ip::tcp::endpoint endpoint) {
  auto executor = co_await asio::this_coro::executor;
  auto acceptor = asio::ip::tcp::acceptor{executor, endpoint};

  for (;;) {
    asio::co_spawn(executor,
        do_session(beast::tcp_stream{co_await acceptor.async_accept()}),
        [](std::exception_ptr e) {
          if (e) {
            try {
              // Check if we got end of stream:
              std::rethrow_exception(e);
            } catch (const std::exception& e) {
              if (auto* system_error =
                      dynamic_cast<const boost::system::system_error*>(&e);
                  system_error &&
                  system_error->code() == http::error::end_of_stream) {
                // We've got end of stream, which happens if HTTP client
                // requested a keep alive, but then went away and closed the
                // socket. We don't mind, but it's not worth it to spam logs
                // with error messages.
                spdlog::trace("Early end of stream");
                return;
              }
              spdlog::error(
                  "Error in session({}): {}", typeid(e).name(), e.what());
            }
          }
        });
  }
}
void Server::serve() {
  spdlog::info("Starting HTTP server");
  const auto address = asio::ip::make_address("0.0.0.0");
  const auto port = 8080;
  asio::co_spawn(ioc_, do_listen(asio::ip::tcp::endpoint{address, port}),
      [](std::exception_ptr e) {
        if (e) {
          try {
            std::rethrow_exception(e);
          } catch (std::exception const& e) {
            spdlog::error("Error trying to listen: {}", e.what());
          }
        }
      });
}
