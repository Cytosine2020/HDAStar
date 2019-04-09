cmake_minimum_required(VERSION 3.10)
project(hw5 C)

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wpedantic -Wextra -Werror -lpthread -pthread")
set(CMAKE_C_FLAGS_DEBUG  "${CMAKE_C_FLAGS_DEBUG} -g")

add_executable(hw5 main.c heap.h heap.c maze.h maze.c node.h node.c compass.h)