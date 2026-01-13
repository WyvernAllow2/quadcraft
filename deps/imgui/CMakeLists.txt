cmake_minimum_required(VERSION 3.10)
project(imgui)

add_library(${PROJECT_NAME}
    src/dcimgui_impl_glfw.cpp
    src/dcimgui_impl_opengl3.cpp
    src/dcimgui.cpp
    src/imgui_demo.cpp
    src/imgui_draw.cpp
    src/imgui_impl_glfw.cpp
    src/imgui_impl_glfw.h
    src/imgui_impl_opengl3_loader.h
    src/imgui_impl_opengl3.cpp
    src/imgui_impl_opengl3.h
    src/imgui_internal.h
    src/imgui_tables.cpp
    src/imgui_widgets.cpp
    src/imgui.cpp
    src/imgui.h
    src/imstb_rectpack.h
    src/imstb_textedit.h
    src/imstb_truetype.h
)

target_include_directories(${PROJECT_NAME} PUBLIC
    include/
)