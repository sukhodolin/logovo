# Boost >= 1.85.0 is required for coroutine support
find_package(Boost 1.85.0 REQUIRED COMPONENTS system)
set (LOGOVO_SOURCES
  main.cc
  server.cc
  server.h
)

find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)

add_executable(logovo ${LOGOVO_SOURCES})

target_compile_definitions(logovo PRIVATE SPDLOG_FMT_EXTERNAL=1)
target_link_libraries(logovo PRIVATE fmt spdlog Boost::system)
