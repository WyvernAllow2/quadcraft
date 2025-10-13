cmake_minimum_required(VERSION 3.10)
project(stb_easy_font)

add_library(${PROJECT_NAME} INTERFACE
    include/stb_easy_font.h
)

target_include_directories(${PROJECT_NAME} INTERFACE
    include/
)