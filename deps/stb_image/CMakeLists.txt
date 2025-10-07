cmake_minimum_required(VERSION 3.10)
project(stb_image)

add_library(${PROJECT_NAME}
    include/stb_image.h
    src/stb_image.c
)

target_include_directories(${PROJECT_NAME} PUBLIC
    include/
)