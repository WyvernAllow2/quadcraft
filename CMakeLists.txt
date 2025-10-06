cmake_minimum_required(VERSION 3.10)
project(quadcraft VERSION 0.1.0 LANGUAGES C)

add_executable(${PROJECT_NAME}
    src/blocks.c
    src/blocks.h
    src/direction.c
    src/direction.h
    src/main.c
    src/math3d.c
    src/math3d.h
    src/utils.c
    src/utils.h
)

set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 99)
target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4
        /permissive-
        /sdl
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -std=c99
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

add_subdirectory(deps/glfw)
add_subdirectory(deps/glad)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glad
)

add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/res ${CMAKE_CURRENT_BINARY_DIR}/res
)

add_dependencies(${PROJECT_NAME} copy_assets)