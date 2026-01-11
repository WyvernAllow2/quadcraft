cmake_minimum_required(VERSION 3.13)
project(quadcraft VERSION 0.1.0 LANGUAGES C)

add_executable(${PROJECT_NAME}
    "src/render/meshing.c"
    "src/render/texture_array.c"
    "src/render/texture_id.c"
    "src/utils/arena.c"
    "src/utils/direction.c"
    "src/utils/math3d.c"
    "src/utils/utils.c"
    "src/world/block_type.c"
    "src/world/camera.c"
    "src/world/chunk.c"
    "src/main.c"
)

target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/src)

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