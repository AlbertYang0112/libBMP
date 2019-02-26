cmake_minimum_required(VERSION 3.9)
project(CVToolBox C)

set(CMAKE_C_STANDARD 99)

add_library(CVToolBox BMPLoader.c BMPLoader.h)