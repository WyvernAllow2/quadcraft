cmake_minimum_required(VERSION 3.13)
project(quadcraft VERSION 0.1.0 LANGUAGES C)

add_executable(${PROJECT_NAME}
    src/arena.c
    src/arena.h
    src/main.c
)

set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 11)
target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4
        /permissive-
        /sdl
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -std=c11
        -Wall
        -Wextra
        -Wpedantic
        -Wsign-conversion
        -Wshadow
        -Wstrict-prototypes
        -Wundef
        -Wpointer-arith
        -Wcast-align
        -Wmissing-prototypes
    )
endif()

set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
add_subdirectory(deps/glfw)
add_subdirectory(deps/glad)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glad
)