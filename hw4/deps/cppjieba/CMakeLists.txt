cmake_minimum_required(VERSION 3.22)

project(cppjieba)

add_library(cppjieba INTERFACE)
target_include_directories(cppjieba INTERFACE include)

add_definitions(-O3 -Wall -g)

if(APPLE) # mac os
  add_definitions(-std=c++0x)
endif()
