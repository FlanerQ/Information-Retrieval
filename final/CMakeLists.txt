cmake_minimum_required(VERSION 3.0)
# set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

project(main)

add_executable(main main.cpp)

set (CMAKE_CXX_STANDARD 20)

include_directories(include)

find_library(LIBSTEMMER NAMES libstemmer.so)

FIND_PACKAGE( Boost 1.81 COMPONENTS serialization REQUIRED )

target_link_libraries(main ${LIBSTEMMER} ${Boost_LIBRARIES})