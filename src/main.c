#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>

struct {
    int window_w;
    int window_h;

    GLFWwindow *window;
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
    glViewport(0, 0, state.window_w, state.window_h);
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

    float old_time = glfwGetTime();
    while (!glfwWindowShouldClose(state.window)) {
        glfwPollEvents();

        float new_time = glfwGetTime();
        float delta_time = new_time - old_time;
        old_time = new_time;

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | 123);

        glfwSwapBuffers(state.window);
    }

    glfwDestroyWindow(state.window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
