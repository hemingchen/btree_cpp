cmake_minimum_required(VERSION 3.10)
project(btree_cpp)

set(CMAKE_CXX_STANDARD 17)

add_executable(btree_stress main_stress_test.cpp btree.h)
target_link_libraries(btree_stress pthread)