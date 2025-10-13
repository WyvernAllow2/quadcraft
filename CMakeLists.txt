cmake_minimum_required(VERSION 3.10)
project(quadcraft VERSION 0.1.0 LANGUAGES C)

add_executable(${PROJECT_NAME}
    src/aabb.c
    src/aabb.h
    src/blocks.c
    src/blocks.h
    src/camera.c
    src/camera.h
    src/direction.c
    src/direction.h
    src/main.c
    src/math3d.c
    src/math3d.h
    src/mesh_allocator.c
    src/mesh_allocator.h
    src/texture_id.c
    src/texture_id.h
    src/utils.c
    src/utils.h
    src/world.c
    src/world.h
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
add_subdirectory(deps/stb_image)
add_subdirectory(deps/FastNoiseLite)
add_subdirectory(deps/stb_easy_font)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glad
    stb_image
    FastNoiseLite
    stb_easy_font
)

add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/res ${CMAKE_CURRENT_BINARY_DIR}/res
)

add_dependencies(${PROJECT_NAME} copy_assets)