cmake_minimum_required(VERSION 3.25)

project(test C)

add_library(test_lib SHARED src/lib.c)
target_include_directories(test_lib PRIVATE include)

add_library(test_exe src/main.c)
target_include_directories(test_lib PRIVATE include)