cmake_minimum_required(VERSION 3.0)
set(CMAKE_TOOLCHAIN_FILE "$ENV{EMSCRIPTEN}/cmake/Modules/Platform/Emscripten.cmake")

project(SimpleGame)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
include_directories(../libs)

# Client executible

add_executable(client main)