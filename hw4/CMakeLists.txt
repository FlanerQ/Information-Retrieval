cmake_minimum_required(VERSION 3.22)

project(hw4_2)

add_executable(hw4_2 hw4_2.cpp)


add_subdirectory(deps/cppjieba)
add_subdirectory(deps/limonp)

target_link_libraries(hw4_2 PRIVATE cppjieba limonp)
