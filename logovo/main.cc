#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <boost/asio/io_context.hpp>

#include "server.h"

namespace asio = boost::asio;

int main(int argc, char* argv[]) {
  spdlog::set_level(spdlog::level::debug);

  // The io_context is required for all I/O
  asio::io_context ioc;

  Server server(ioc);
  server.serve();

  return 0;
}
