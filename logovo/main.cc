#include <spdlog/spdlog.h>

#include "server.h"

int main(int argc, char* argv[]) {
  spdlog::set_level(spdlog::level::trace);

  Server server;
  server.serve();

  return 0;
}
