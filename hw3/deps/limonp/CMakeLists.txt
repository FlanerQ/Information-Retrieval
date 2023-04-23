cmake_minimum_required(VERSION 3.22)

PROJECT(limonp)

add_library(limonp INTERFACE)
target_include_directories(limonp INTERFACE include)

add_definitions(-O3 -Wall -g)

if(APPLE) # mac os
  add_definitions(-std=c++0x)
endif()
