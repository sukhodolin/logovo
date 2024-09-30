#include <liblogovo/handler.h>
#include <liblogovo/server.h>
#include <spdlog/spdlog.h>

#include <boost/program_options.hpp>
#include <iostream>

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
  bool trace;
  std::string log_root;
  std::string listen_at;
  ushort port;

  po::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help", "produce help message")
    ("trace", po::bool_switch(&trace)->default_value(false),
      "enable trace logs")
    ("log-root", po::value<std::string>(&log_root)->default_value("."),
      "log root directory to serve logs from")
    ("listen-at", po::value<std::string>(&listen_at)->default_value("127.0.0.1"),
      "network address to listen at")
    ("port", po::value<ushort>(&port)->default_value(8080),
      "network port to listen at");
  // clang-format on
  po::positional_options_description p;
  p.add("log-root", 1);
  po::variables_map vm;

  try {
    po::store(
        po::command_line_parser(argc, argv).options(desc).positional(p).run(),
        vm);
    po::notify(vm);
    if (vm.count("help")) {
      std::cout << desc << "\n";
      return 1;
    }

    if (trace) {
      spdlog::set_level(spdlog::level::trace);
    } else {
      spdlog::set_level(spdlog::level::info);
    }

    Handler handler(std::filesystem::canonical(log_root));
    Server server(handler, listen_at, port);
    server.serve();
  } catch (const std::exception& e) {
    spdlog::error(e.what());
    return 1;
  }

  return 0;
}
