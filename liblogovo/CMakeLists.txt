# Boost >= 1.85.0 is required for coroutine support
find_package(Boost 1.85.0 REQUIRED COMPONENTS system url)
set (LOGOVO_SOURCES
  vendor/generator.h
  handler.cc
  handler.h
  server.cc
  server.h
  tail.h
  tail.cc
)

find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)

add_library(liblogovo OBJECT ${LOGOVO_SOURCES})

target_link_libraries(liblogovo PRIVATE fmt spdlog Boost::system Boost::url)
target_include_directories(liblogovo INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
