cmake_minimum_required(VERSION 3.0)
set(CMAKE_TOOLCHAIN_FILE "$ENV{EMSCRIPTEN}/cmake/Modules/Platform/Emscripten.cmake")

project(SimpleGame)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fno-exceptions -Wall -Wextra")
include_directories(../libs)
include_directories(..)

# Client executible

add_executable(client client ../libs/json11/json11.cpp)

set_target_properties(client PROPERTIES LINK_FLAGS "--js-library ${CMAKE_CURRENT_LIST_DIR}/../libs/DataChannelServer/client-src/internal-api.js")