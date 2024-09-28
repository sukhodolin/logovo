#pragma once

#include <boost/asio.hpp>

class Server {
 public:
  Server(boost::asio::io_context& ioc);

  void serve();

 private:
  boost::asio::io_context& ioc_;
};
