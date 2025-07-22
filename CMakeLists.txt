# rjcpt top-level CMakeLists.txt

cmake_minimum_required(VERSION 3.24)

project(rjcpt)

set(CMAKE_CXX_STANDARD 23)

set(rjcpt_VERSION_MAJOR 0)
set(rjcpt_VERSION_MINOR 1)

include(GenerateExportHeader)

add_subdirectory(core)
add_subdirectory(gui)
add_subdirectory(exec)
add_subdirectory(gui-exec)
