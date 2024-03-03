cmake_minimum_required(VERSION 3.20)

project(test C)

add_library(test_lib SHARED src/lib.c)
target_include_directories(test_lib PRIVATE include)
add_library(test_lib2 SHARED src/lib2.c)
target_include_directories(test_lib2 PRIVATE include)

add_executable(test_exe src/main.c)
target_include_directories(test_exe PRIVATE include)
target_link_libraries(test_exe test_lib test_lib2)
