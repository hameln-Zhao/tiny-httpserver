cmake_minimum_required(VERSION 3.19)
project(newhttpserver C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-pthread")
add_executable(newhttpserver main.c)