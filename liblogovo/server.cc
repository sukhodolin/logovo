#include "server.h"

#include <spdlog/spdlog.h>

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <list>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

using namespace boost::asio::experimental::awaitable_operators;

using session_state = std::shared_ptr<beast::tcp_stream>;
using handle = std::weak_ptr<session_state::element_type>;

Server::Server() : ioc_() {}

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

asio::awaitable<void> do_session(session_state s) {
  // This buffer is required to persist across reads
  beast::flat_buffer buffer;

  for (;;) {
    // Set the timeout.
    s->expires_after(std::chrono::seconds(30));

    // Read a request
    http::request<http::string_body> req;
    co_await http::async_read(*s, buffer, req);
    spdlog::info("Request: {}", req.method_string().data());
    // Handle the request
    http::message_generator msg = handle_request(std::move(req));

    // Determine if we should close the connection
    bool keep_alive = msg.keep_alive();

    // Send the response
    co_await beast::async_write(*s, std::move(msg));

    if (!keep_alive) {
      // This means we should close the connection, usually because
      // the response indicated the "Connection: close" semantic.
      break;
    }
  }

  // Send a TCP shutdown
  s->socket().shutdown(asio::ip::tcp::socket::shutdown_send);

  // At this point the connection is closed gracefully. We ignore the error
  // because the client might have dropped the connection already.
}

void Server::serve() {
  spdlog::info("Starting HTTP server");
  const auto address = asio::ip::make_address("0.0.0.0");
  const auto port = 8080;
  const auto endpoint = asio::ip::tcp::endpoint{address, port};

  asio::signal_set signals(ioc_, SIGINT, SIGTERM);

  std::list<handle> handles;
  asio::co_spawn(
      ioc_,
      [&]() -> asio::awaitable<void> {
        auto executor = co_await asio::this_coro::executor;
        for (asio::ip::tcp::acceptor acceptor(executor, endpoint);;) {
          session_state s = std::make_shared<beast::tcp_stream>(
              co_await acceptor.async_accept(
                  make_strand(executor), asio::use_awaitable));

          handles.remove_if(std::mem_fn(&handle::expired));
          handles.emplace_back(s);
          co_spawn(executor, do_session(s), [](std::exception_ptr e) {
            try {
              if (e) {
                std::rethrow_exception(e);
              }
            } catch (const std::exception& e) {
              // Check if we got end of stream:
              if (auto* system_error =
                      dynamic_cast<const boost::system::system_error*>(&e);
                  system_error &&
                  system_error->code() == http::error::end_of_stream) {
                // We've got end of stream, which happens if HTTP client
                // requested a keep alive, but then went away and closed the
                // socket. We don't mind, but it's not worth it to spam logs
                // with error messages, so `trace` instead of `error`.
                spdlog::trace("Early end of stream");
                return;
              }

              spdlog::error("Error in session: {}", e.what());
            }
          });
        }
      }() || signals.async_wait(asio::use_awaitable),
      [&handles](std::exception_ptr e, auto result) {
        try {
          if (e) {
            std::rethrow_exception(e);
          }
          auto signal = std::get<1>(result);
          spdlog::info("Got signal, shutting down");

          for (auto h : handles)
            if (auto s = h.lock()) {
              spdlog::info("Shutting down live session {}:{}",
                  s->socket().remote_endpoint().address().to_string(),
                  s->socket().remote_endpoint().port());
              post(s->get_executor(), [s] { s->cancel(); });
            }
        } catch (const std::exception& e) {
          spdlog::error("Error trying to listen: {}", e.what());
        }
      });

  ioc_.join();
}
