cmake_minimum_required(VERSION 3.25)
project(solink C)
set(CMAKE_C_STANDARD 11)

add_executable(solink
    src/main.c
    src/elf.c
)

target_include_directories(solink PRIVATE include/)
