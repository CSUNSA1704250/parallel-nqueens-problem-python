cmake_minimum_required(VERSION 3.5)
project(project_name VERSION 0.0.1)

# Export commands
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

 # C++ standard requirement
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SRC_FILES src/*.cc)

add_executable(nqueens ${SRC_FILES})

target_compile_options(nqueens PUBLIC)
#target_compile_options(nqueens PUBLIC -O3 -march=native)

#find_package(fmt REQUIRED)
find_package(Threads REQUIRED)
#find_package(OpenMP REQUIRED)

#target_link_libraries(nqueens PUBLIC fmt::fmt)
#target_link_libraries(nqueens PUBLIC OpenMP::OpenMP_CXX)
target_link_libraries(nqueens PUBLIC ${CMAKE_THREAD_LIBS_INIT})
