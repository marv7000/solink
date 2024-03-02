cmake_minimum_required(VERSION 3.20)
project(solink C)
set(CMAKE_C_STANDARD 11)

set(SOLINK_VER_MAJ 0)
set(SOLINK_VER_MIN 1)
string(TIMESTAMP SOLINK_VER_PATCH "%Y/%m/%d, %H:%M:%S")

add_executable(solink
    src/main.c
    src/args.c
    src/elf.c
    src/patch.c
)

target_compile_definitions(solink PRIVATE SOLINK_VER_MAJ="${SOLINK_VER_MAJ}")
target_compile_definitions(solink PRIVATE SOLINK_VER_MIN="${SOLINK_VER_MIN}")
target_compile_definitions(solink PRIVATE SOLINK_VER_PATCH="${SOLINK_VER_PATCH}")
target_compile_definitions(solink PRIVATE SOLINK_ARCH_${CMAKE_HOST_SYSTEM_PROCESSOR})

target_include_directories(solink PRIVATE include/)
install(TARGETS solink)
