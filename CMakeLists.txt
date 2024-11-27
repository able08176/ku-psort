cmake_minimum_required(VERSION 3.30)
project(ku_psort C)

set(CMAKE_C_STANDARD 11)

add_executable(main main.c)
add_executable(ku_psort ku_psort.c)
