#pragma once

#include <boost/asio.hpp>

class Server {
 public:
  Server(std::string listen_at, ushort port);

  void serve();

 private:
  std::string listen_at_;
  ushort port_;
  boost::asio::thread_pool ioc_;
};
