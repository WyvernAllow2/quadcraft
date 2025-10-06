#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "math3d.h"
#include "utils.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>

#define CAMERA_SPEED 16.0f
#define MOUSE_SENSITIVITY 0.125f

typedef struct Camera {
    Vec3 position;
    float pitch;
    float yaw;

    float fov;
    float aspect;
    float znear;
    float zfar;
} Camera;

typedef struct Vertex {
    Vec3 position;
} Vertex;

struct {
    int window_w;
    int window_h;

    GLFWwindow *window;

    GLuint shader;
    GLuint vao;
    GLuint vbo;
    GLuint ebo;

    Camera camera;
    float old_mouse_x;
    float old_mouse_y;
    bool first_mouse;
} state;

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
    (void)source;
    (void)id;
    (void)length;
    (void)user_param;
    const char *type_str = get_type_string(type);
    const char *severity_str = get_severity_string(severity);
    fprintf(stderr, "OpenGL (Type: %s, Severity: %s): %s\n", type_str, severity_str, message);
}

static void window_size_callback(GLFWwindow *window, int width, int height) {
    (void)window;
    assert(window == state.window);

    state.window_w = width;
    state.window_h = height;
    state.camera.aspect = state.window_w / (float)state.window_h;
    glViewport(0, 0, state.window_w, state.window_h);
}

static void cursor_pos_callback(GLFWwindow *window, double xpos, double ypos) {
    (void)window;
    assert(window == state.window);

    if (state.first_mouse) {
        state.old_mouse_x = xpos;
        state.old_mouse_y = ypos;
        state.first_mouse = false;
    }

    float delta_x = xpos - state.old_mouse_x;
    float delta_y = ypos - state.old_mouse_y;

    state.old_mouse_x = xpos;
    state.old_mouse_y = ypos;

    state.camera.yaw += to_radians(delta_x * MOUSE_SENSITIVITY);
    state.camera.pitch += to_radians(-delta_y * MOUSE_SENSITIVITY);

    state.camera.pitch = clamp(state.camera.pitch, -HALF_PI + 1e-6f, HALF_PI - 1e-6);
    state.camera.yaw = fmodf(state.camera.yaw + TAU, TAU);
}

static void uniform_mat4(GLuint program, const char *name, const Mat4 *value) {
    glUniformMatrix4fv(glGetUniformLocation(program, name), 1, GL_FALSE, value->data);
}

int main(void) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        fprintf(stderr, "glfwInit() failed\n");
        return EXIT_FAILURE;
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
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(state.window);

    if (!gladLoadGL(glfwGetProcAddress)) {
        fprintf(stderr, "gladLoadGL() failed\n");
        return EXIT_FAILURE;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(gl_debug_output, NULL);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, true);

    glfwSetWindowSizeCallback(state.window, window_size_callback);

    state.first_mouse = true;
    glfwSetInputMode(state.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(state.window, cursor_pos_callback);
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(state.window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    state.shader = compile_program_from_files("res/shaders/chunk.vert", "res/shaders/chunk.frag");
    if (!state.shader) {
        fprintf(stderr, "compile_program_from_files() failed\n");
        return EXIT_FAILURE;
    }

    Vertex vertices[] = {
        {{0.5f, 0.5f, 0.0f}},
        {{0.5f, -0.5f, 0.0f}},
        {{-0.5f, -0.5f, 0.0f}},
        {{-0.5f, 0.5f, 0.0f}},
    };

    uint32_t indices[] = {0, 1, 3, 1, 2, 3};

    glGenVertexArrays(1, &state.vao);
    glGenBuffers(1, &state.vbo);
    glGenBuffers(1, &state.ebo);

    glBindVertexArray(state.vao);

    glBindBuffer(GL_ARRAY_BUFFER, state.vbo);
    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(Vertex), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(uint32_t), indices, GL_STATIC_DRAW);

    /* Position attribute */
    glEnableVertexAttribArray(0);
    const void *position_offset = offsetof(Vertex, position);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), position_offset);

    glBindVertexArray(0);

    state.camera = (Camera){
        .position = {0, 0, 0},
        .pitch = 0.0f,
        .yaw = to_radians(90.0f),
        .fov = to_radians(90.0f),
        .aspect = mode->width / (float)mode->height,
        .znear = 0.1f,
        .zfar = 500.0f,
    };

    float old_time = glfwGetTime();
    while (!glfwWindowShouldClose(state.window)) {
        glfwPollEvents();

        float new_time = glfwGetTime();
        float delta_time = new_time - old_time;
        old_time = new_time;

        const Vec3 cam_forward = vec3_normalize((Vec3){
            .x = cosf(state.camera.yaw) * cosf(state.camera.pitch),
            .y = sinf(state.camera.pitch),
            .z = sinf(state.camera.yaw) * cosf(state.camera.pitch),
        });

        const Vec3 cam_right = vec3_normalize(vec3_cross(cam_forward, (Vec3){0, 1, 0}));
        const Vec3 cam_up = vec3_normalize(vec3_cross(cam_right, cam_forward));

        Vec3 wish_dir = {0};
        if (glfwGetKey(state.window, GLFW_KEY_W)) {
            wish_dir = vec3_add(wish_dir, cam_forward);
        }

        if (glfwGetKey(state.window, GLFW_KEY_S)) {
            wish_dir = vec3_sub(wish_dir, cam_forward);
        }

        if (glfwGetKey(state.window, GLFW_KEY_D)) {
            wish_dir = vec3_add(wish_dir, cam_right);
        }

        if (glfwGetKey(state.window, GLFW_KEY_A)) {
            wish_dir = vec3_sub(wish_dir, cam_right);
        }

        state.camera.position = vec3_add(
            state.camera.position, vec3_scale(vec3_normalize(wish_dir), CAMERA_SPEED * delta_time));

        Mat4 view;
        mat4_look_at(&view, state.camera.position, vec3_add(state.camera.position, cam_forward),
                     cam_up);
        Mat4 proj;
        mat4_perspective(&proj, state.camera.fov, state.camera.aspect, state.camera.znear,
                         state.camera.zfar);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(state.vao);
        glUseProgram(state.shader);

        uniform_mat4(state.shader, "u_view", &view);
        uniform_mat4(state.shader, "u_proj", &proj);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);

        glUseProgram(0);
        glBindVertexArray(0);

        glfwSwapBuffers(state.window);
    }

    glDeleteBuffers(1, &state.ebo);
    glDeleteBuffers(1, &state.vbo);
    glDeleteVertexArrays(1, &state.vao);
    glDeleteProgram(state.shader);

    glfwDestroyWindow(state.window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
