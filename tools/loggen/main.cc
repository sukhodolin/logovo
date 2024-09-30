#include <spdlog/spdlog.h>

#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
  std::string path;
  size_t num_lines;

  po::options_description desc("Allowed options");
  // clang-format off
  desc.add_options()
    ("help", "produce help message")
    ("path", po::value<std::string>(&path), "path to the file to generate")
    ("lines-number", po::value<size_t>(&num_lines)->default_value(10), "the amount of log lines to generate");
  // clang-format on
  po::positional_options_description p;
  p.add("path", 1);
  po::variables_map vm;
  po::store(
      po::command_line_parser(argc, argv).options(desc).positional(p).run(),
      vm);
  po::notify(vm);

  if (vm.count("help") || !vm.count("path")) {
    std::cout << desc << "\n";
    return 1;
  }

  spdlog::info("generating file at {} with {} lines", path, num_lines);
  std::ofstream output(path);
  if (output.fail()) {
    spdlog::error("failed to open file at {} for writing", path);
    return 1;
  }

  for (size_t i = 0; i < num_lines; ++i) {
    output << fmt::format("I'm line number {} of {}\n", i, num_lines);
  }
  output.flush();

  return 0;
}
