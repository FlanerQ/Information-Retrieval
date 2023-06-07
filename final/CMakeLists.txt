cmake_minimum_required(VERSION 3.0)
project(main)

add_executable(main main.cpp)

set (CMAKE_CXX_STANDARD 20)

include_directories(include)

find_library(LIBSTEMMER NAMES libstemmer.so)

target_link_libraries(main ${LIBSTEMMER})