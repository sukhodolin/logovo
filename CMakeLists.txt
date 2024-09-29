cmake_minimum_required(VERSION 3.29)
project(logovo)

set(CMAKE_CXX_STANDARD 23)

add_compile_definitions(SPDLOG_FMT_EXTERNAL=1)

add_subdirectory(liblogovo)
add_subdirectory(tests)

add_executable(logovo main.cc)
target_link_libraries(logovo PRIVATE liblogovo)

install(TARGETS logovo)
