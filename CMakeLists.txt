cmake_minimum_required(VERSION 3.29)
project(logovo)

set(CMAKE_CXX_STANDARD 23)

add_compile_definitions(SPDLOG_FMT_EXTERNAL=1)

add_subdirectory(liblogovo)
add_subdirectory(tests)
add_subdirectory(tools)

find_package(Boost REQUIRED COMPONENTS program_options)
add_executable(logovo main.cc)
target_link_libraries(logovo PRIVATE liblogovo Boost::program_options)

install(TARGETS logovo loggen)
