#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "camera.h"
#include "chunk.h"
#include "meshing.h"
#include "texture_array.h"
#include "utils.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>

#define DEFAULT_CAMERA_SPEED 16.0f
#define DEFAULT_MOUSE_SENSITIVITY 0.25f

static void glfw_error_callback(int error_code, const char *description) {
    (void)error_code;
    fprintf(stderr, "GLFW: %s\n", description);
}

static const char *get_severity_string(GLenum severity) {
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
        return "High";
    case GL_DEBUG_SEVERITY_MEDIUM:
        return "Medium";
    case GL_DEBUG_SEVERITY_LOW:
        return "Low";
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        return "Notification";
    default:
        return "Unknown";
    }
}

static const char *get_type_string(GLenum type) {
    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        return "Error";
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        return "Deprecated Behaviour";
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        return "Undefined Behaviour";
    case GL_DEBUG_TYPE_PORTABILITY:
        return "Portability";
    case GL_DEBUG_TYPE_PERFORMANCE:
        return "Performance";
    case GL_DEBUG_TYPE_MARKER:
        return "Marker";
    case GL_DEBUG_TYPE_PUSH_GROUP:
        return "Push Group";
    case GL_DEBUG_TYPE_POP_GROUP:
        return "Pop Group";
    case GL_DEBUG_TYPE_OTHER:
        return "Other";
    default:
        return "Unknown";
    }
}

static void APIENTRY gl_debug_output(GLenum source, GLenum type, unsigned int id, GLenum severity,
                                     GLsizei length, const char *message, const void *user_param) {
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) {
        return;
    }

    (void)source;
    (void)id;
    (void)length;
    (void)user_param;
    const char *type_str = get_type_string(type);
    const char *severity_str = get_severity_string(severity);
    fprintf(stderr, "OpenGL (Type: %s, Severity: %s): %s\n", type_str, severity_str, message);
}

struct {
    int window_w;
    int window_h;
    GLFWwindow *window;

    Camera camera;
    float camera_speed;
    float mouse_sensitivity;

    bool first_mouse;
    float prev_mouse_x;
    float prev_mouse_y;

    GLuint shader;
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    GLuint texture_array;

    Chunk chunk;
    uint32_t quad_count;
} state;

static void window_size_callback(GLFWwindow *window, int width, int height) {
    (void)window;
    assert(window == state.window);

    state.window_w = width;
    state.window_h = height;
    glViewport(0, 0, state.window_w, state.window_h);
}

static void cursor_pos_callback(GLFWwindow *window, double xpos, double ypos) {
    (void)window;
    assert(window == state.window);

    if (state.first_mouse) {
        state.prev_mouse_x = xpos;
        state.prev_mouse_y = ypos;
        state.first_mouse = false;
    }

    float delta_x = xpos - state.prev_mouse_x;
    float delta_y = ypos - state.prev_mouse_y;

    state.prev_mouse_x = xpos;
    state.prev_mouse_y = ypos;

    state.camera.yaw += to_radians(delta_x * state.mouse_sensitivity);
    state.camera.pitch += to_radians(-delta_y * state.mouse_sensitivity);
}

static bool on_init(void) {
    Arena init_arena;
    arena_create(&init_arena, MIB_TO_BYTES(10));

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        fprintf(stderr, "glfwInit() failed\n");
        return false;
    }

    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);

    state.window_w = mode->width;
    state.window_h = mode->height;

    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    state.window = glfwCreateWindow(state.window_w, state.window_h, "Quadcraft", monitor, NULL);
    if (!state.window) {
        fprintf(stderr, "glfwCreateWindow() failed\n");
        return false;
    }

    glfwMakeContextCurrent(state.window);

    if (!gladLoadGL(glfwGetProcAddress)) {
        fprintf(stderr, "gladLoadGL() failed\n");
        return false;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(gl_debug_output, NULL);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, true);

    glfwSetWindowSizeCallback(state.window, window_size_callback);

    glfwSetInputMode(state.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(state.window, cursor_pos_callback);

    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(state.window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    state.camera = (Camera){
        .fov = to_radians(90.0f),
        .aspect = state.window_w / (float)state.window_h,
        .znear = 0.001f,
        .zfar = 1000.0f,
        .yaw = -to_radians(90.0f),
    };

    state.camera_speed = DEFAULT_CAMERA_SPEED;
    state.mouse_sensitivity = DEFAULT_MOUSE_SENSITIVITY;
    state.first_mouse = true;

    camera_update(&state.camera);

    state.shader =
        compile_program_from_files("res/shaders/chunk.vert", "res/shaders/chunk.frag", &init_arena);
    if (!state.shader) {
        fprintf(stderr, "compile_program_from_files() failed\n");
        return false;
    }

    for (int z = 0; z < CHUNK_SIZE; z++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int x = 0; x < CHUNK_SIZE; x++) {
                Block_Type type = BLOCK_DIRT;
                if (y > 8) {
                    type = BLOCK_AIR;
                } else if (y == 8) {
                    type = BLOCK_GRASS;
                }

                iVec3 pos = {x, y, z};
                chunk_set_block_unsafe(&state.chunk, pos, type);
            }
        }
    }

    uint32_t index_count = 0;
    uint32_t *indices = generate_index_buffer(&index_count, &init_arena);

    uint32_t vertex_count = 0;
    uint32_t *vertices = mesh_chunk(&state.chunk, &vertex_count, &init_arena);

    glGenVertexArrays(1, &state.vao);
    glGenBuffers(1, &state.vbo);
    glGenBuffers(1, &state.ebo);

    glBindVertexArray(state.vao);

    glBindBuffer(GL_ARRAY_BUFFER, state.vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizei)(sizeof(uint32_t) * vertex_count), vertices,
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizei)(sizeof(uint32_t) * index_count), indices,
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(uint32_t), NULL);

    state.texture_array = load_texture_array();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_FRAMEBUFFER_SRGB);

    state.quad_count = (vertex_count / 4);

    arena_destroy(&init_arena);
    return true;
}

static void on_quit(void) {
    glDeleteBuffers(1, &state.ebo);
    glDeleteBuffers(1, &state.vbo);
    glDeleteVertexArrays(1, &state.vao);

    glfwDestroyWindow(state.window);
    glfwTerminate();
}

static void on_update(float delta_time) {
    (void)delta_time;

    Vec3 move_dir = {0};
    if (glfwGetKey(state.window, GLFW_KEY_W)) {
        move_dir = vec3_add(move_dir, state.camera.forward);
    }
    if (glfwGetKey(state.window, GLFW_KEY_S)) {
        move_dir = vec3_sub(move_dir, state.camera.forward);
    }
    if (glfwGetKey(state.window, GLFW_KEY_D)) {
        move_dir = vec3_add(move_dir, state.camera.right);
    }
    if (glfwGetKey(state.window, GLFW_KEY_A)) {
        move_dir = vec3_sub(move_dir, state.camera.right);
    }

    Vec3 move_amount = vec3_scale(vec3_normalize(move_dir), delta_time * state.camera_speed);
    state.camera.position = vec3_add(state.camera.position, move_amount);

    camera_update(&state.camera);
}

static void on_draw(float delta_time) {
    (void)delta_time;
    glClearColor(0.2, 0.2, 0.2, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(state.shader);
    glUniformMatrix4fv(glGetUniformLocation(state.shader, "u_view_proj"), 1, GL_FALSE,
                       state.camera.view_proj.data);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, state.texture_array);
    glUniform1i(glGetUniformLocation(state.shader, "u_texture_array"), 0);

    glBindVertexArray(state.vao);
    glDrawElements(GL_TRIANGLES, (GLsizei)(state.quad_count * 6), GL_UNSIGNED_INT, 0);

    glfwSwapBuffers(state.window);
}

int main(void) {
    if (!on_init()) {
        fprintf(stderr, "Failed to initialize\n");
        return EXIT_FAILURE;
    }

    float prev_time = glfwGetTime();

    while (!glfwWindowShouldClose(state.window)) {
        glfwPollEvents();

        float curr_time = glfwGetTime();
        float delta_time = curr_time - prev_time;
        prev_time = curr_time;

        on_update(delta_time);
        on_draw(delta_time);
    }

    on_quit();
    return EXIT_SUCCESS;
}
