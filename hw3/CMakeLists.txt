cmake_minimum_required(VERSION 3.22)

project(hw3)

add_executable(hw3 hw3_1.cpp)


add_subdirectory(deps/cppjieba)
add_subdirectory(deps/limonp)

target_link_libraries(hw3 PRIVATE cppjieba limonp)
