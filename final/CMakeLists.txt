cmake_minimum_required(VERSION 3.0)
# set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(main)

add_executable(main main.cpp)

set (CMAKE_CXX_STANDARD 20)

include_directories(include)

find_package(nlohmann_json REQUIRED)

find_library(LIBSTEMMER NAMES libstemmer.so)

find_package(Boost 1.81 COMPONENTS system filesystem serialization REQUIRED)

target_link_libraries(main ${LIBSTEMMER} ${Boost_LIBRARIES} nlohmann_json::nlohmann_json)