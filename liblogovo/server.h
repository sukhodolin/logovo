#pragma once

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>

class Handler;

class Server {
 public:
  // Handler must be alive for the whole server lifetime
  Server(Handler& handler, std::string listen_at, ushort port);

  void serve();

 private:
  using session_state = std::shared_ptr<boost::beast::tcp_stream>;
  using handle = std::weak_ptr<session_state::element_type>;

  boost::asio::awaitable<void> session_(session_state s);
  Handler& handler_;
  std::string listen_at_;
  ushort port_;
  boost::asio::thread_pool ioc_;
};
