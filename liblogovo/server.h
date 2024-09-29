#pragma once

#include <boost/asio.hpp>

class Server {
 public:
  Server();

  void serve();

 private:
  // boost::asio::awaitable<void> listen_(boost::asio::ip::tcp::endpoint
  // endpoint);
  boost::asio::thread_pool ioc_;
};
