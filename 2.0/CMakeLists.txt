cmake_minimum_required (VERSION 3.8)
set (CMAKE_CXX_STANDARD 20)
project ("John Conway's Game of Life")
add_executable (CMakeTarget main.cpp life.h life.cpp)