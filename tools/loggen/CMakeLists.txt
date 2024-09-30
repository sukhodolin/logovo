find_package(Boost REQUIRED COMPONENTS program_options)

add_executable(loggen main.cc)
target_link_libraries(loggen PRIVATE fmt spdlog Boost::program_options)
