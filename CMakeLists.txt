cmake_minimum_required(VERSION 3.29)
project(logovo)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(logovo)

install(TARGETS logovo)
