cmake_minimum_required(VERSION 3.5)
project(project_name VERSION 0.0.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SRC_FILES src/*.cc)

add_executable(nqueens ${SRC_FILES})

set_property(TARGET nqueens PROPERTY CXX_STANDARD 17)
target_compile_options(nqueens PUBLIC -O3 -march=native)

find_package(fmt REQUIRED)
target_link_libraries(nqueens PUBLIC fmt::fmt)

find_package(Threads REQUIRED)
target_link_libraries(nqueens PUBLIC ${CMAKE_THREAD_LIBS_INIT})
