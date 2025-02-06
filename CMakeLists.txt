cmake_minimum_required(VERSION 3.10)
project(BoostServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system thread)

add_executable(server main.cpp game_protoc.cpp)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

target_include_directories(server PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(server PRIVATE Boost::system Boost::thread)

