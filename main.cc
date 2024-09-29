#include <liblogovo/server.h>
#include <linux/io_uring.h>
#include <spdlog/spdlog.h>

int main(int argc, char* argv[]) {
  spdlog::set_level(spdlog::level::trace);

  Server server;
  server.serve();

  return 0;
}
