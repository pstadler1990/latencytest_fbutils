cmake_minimum_required(VERSION 3.13)
project(fb C)

set(CMAKE_C_STANDARD 99)

add_executable(fb main.c fb_lib.c fb_lib.h)