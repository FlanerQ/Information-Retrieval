cmake_minimum_required(VERSION 3.22)

project(hw3_1)

add_executable(hw3_1 hw3_1.cpp)


add_subdirectory(deps/cppjieba)
add_subdirectory(deps/limonp)

target_link_libraries(hw3_1 PRIVATE cppjieba limonp)
