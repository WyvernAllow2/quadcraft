cmake_minimum_required(VERSION 3.13)
project(quadcraft VERSION 0.1.0 LANGUAGES C)

add_executable(${PROJECT_NAME}
    src/arena.c
    src/arena.h
    src/block_type.c
    src/block_type.h
    src/camera.c
    src/camera.h
    src/chunk.c
    src/chunk.h
    src/direction.c
    src/direction.h
    src/main.c
    src/math3d.c
    src/math3d.h
    src/meshing.c
    src/meshing.h
    src/utils.c
    src/utils.h
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
add_subdirectory(deps/stb_image)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glad
    stb_image
)