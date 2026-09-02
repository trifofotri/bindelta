cmake_minimum_required(VERSION 3.16)
project(bindelta CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CAPSTONE_ARCHITECTURE_DEFAULT OFF CACHE BOOL "" FORCE)
set(CAPSTONE_X86_SUPPORT ON CACHE BOOL "" FORCE)
add_subdirectory(third_party/capstone)

add_executable(bindelta
    src/bindelta.cpp
    src/binary/elf_file.cpp
    src/diff/byte_diff.cpp
    src/render/asm_renderer.cpp
)

target_include_directories(bindelta PRIVATE include)
target_link_libraries(bindelta PRIVATE capstone)