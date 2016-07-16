cmake_minimum_required(VERSION 3.0)
project(SimpleGame)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wextra")

find_package(Boost 1.58.0 REQUIRED COMPONENTS system)

include_directories(${Boost_INCLUDE_DIRS} ../libs ..)

link_directories(server PUBLIC ../libs)

# Client executible

add_executable(server server ../libs/json11/json11.cpp)

target_include_directories(server PUBLIC src)

target_link_libraries(server ${Boost_LIBRARIES} DataChannelServer)