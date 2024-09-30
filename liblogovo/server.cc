#include "server.h"

#include <spdlog/spdlog.h>

#include <boost/asio.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/experimental/awaitable_operators.hpp>
#include <boost/beast/http.hpp>
#include <list>

#include "handler.h"

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

using namespace boost::asio::experimental::awaitable_operators;

Server::Server(Handler& handler, std::string listen_at, ushort port)
    : handler_(handler), listen_at_(listen_at), port_(port) {}

asio::awaitable<void> Server::session_(session_state s) {
  // This buffer is required to persist across reads
  beast::flat_buffer buffer;

  for (;;) {
    // Read a request
    http::request<http::string_body> req;
    co_await http::async_read(*s, buffer, req);
    // Handle the request
    http::message_generator msg = handler_.handle_request(std::move(req));

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
  spdlog::info("Starting HTTP server listening at {}:{}", listen_at_, port_);
  const auto address = asio::ip::make_address(listen_at_);
  const auto endpoint = asio::ip::tcp::endpoint{address, port_};

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
          co_spawn(executor, session_(s), [](std::exception_ptr e) {
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
              spdlog::info("Waiting for the live session to shut down {}:{}",
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
