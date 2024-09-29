# Boost >= 1.85.0 is required for coroutine support
find_package(Boost 1.85.0 REQUIRED COMPONENTS system)
set (LOGOVO_SOURCES
  server.cc
  server.h
)

find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)

add_library(liblogovo OBJECT ${LOGOVO_SOURCES})

target_link_libraries(liblogovo PRIVATE fmt spdlog Boost::system)
target_include_directories(liblogovo INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
