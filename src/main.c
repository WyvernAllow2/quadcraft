#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "direction.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>

#include "block_type.h"
#include "camera.h"
#include "chunk.h"
#include "math3d.h"
#include "utils.h"

static GLuint compile_shader(const char *source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint did_compile;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &did_compile);
    if (!did_compile) {
        char info_log[256];
        glGetShaderInfoLog(shader, sizeof(info_log), NULL, info_log);
        fprintf(stderr, "Shader compilation failed: %s\n", info_log);

        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

static GLuint compile_program(GLuint vert, GLuint frag) {
    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    glDeleteShader(vert);
    glDeleteShader(frag);

    GLint did_link;
    glGetProgramiv(program, GL_LINK_STATUS, &did_link);
    if (!did_link) {
        char info_log[256];
        glGetProgramInfoLog(program, sizeof(info_log), NULL, info_log);
        fprintf(stderr, "Shader linking failed: %s\n", info_log);

        glDeleteProgram(program);
        return 0;
    }

    return program;
}

static GLuint compile_program_from_files(const char *vert_filename, const char *frag_filename) {
    const char *vert_source = slurp_file_str(vert_filename);
    if (!vert_source) {
        fprintf(stderr, "Failed to load shader source: %s\n", vert_filename);
        return 0;
    }

    const char *frag_source = slurp_file_str(frag_filename);
    if (!frag_source) {
        fprintf(stderr, "Failed to load shader source: %s\n", frag_filename);
        return 0;
    }

    GLuint vert = compile_shader(vert_source, GL_VERTEX_SHADER);
    if (!vert) {
        fprintf(stderr, "Failed to compile shader: %s\n", vert_filename);
        return 0;
    }

    GLuint frag = compile_shader(frag_source, GL_FRAGMENT_SHADER);
    if (!frag) {
        fprintf(stderr, "Failed to compile shader: %s\n", frag_filename);
        return 0;
    }

    return compile_program(vert, frag);
}

typedef struct Block_Vertex {
    Vec3 position;
} Block_Vertex;

static Camera camera;
static const float CAMERA_SPEED = 5.0f;
static const float MOUSE_SENSITIVITY = 0.25f;

static bool first_mouse = true;
static float last_x;
static float last_y;

static void cursor_callback(GLFWwindow *window, double x_pos, double y_pos) {
    (void)window;
    if (first_mouse) {
        last_x = x_pos;
        last_y = y_pos;
        first_mouse = false;
    }

    float delta_x = x_pos - last_x;
    float delta_y = y_pos - last_y;

    last_x = x_pos;
    last_y = y_pos;

    camera.pitch += -to_radians(delta_y * MOUSE_SENSITIVITY);
    camera.yaw += to_radians(delta_x * MOUSE_SENSITIVITY);
}

static void error_callback(int error_code, const char *description) {
    (void)error_code;
    fprintf(stderr, "GLFW: %s\n", description);
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    (void)scancode;
    (void)mods;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

#define MAX_VISIBLE_FACES ((CHUNK_VOLUME / 2) * 6)
#define MAX_VERTICES (MAX_VISIBLE_FACES * 4)
#define MAX_INDICES (MAX_VISIBLE_FACES * 6)

/* clang-format off */
static const Vec3 FACE_VERTEX_TABLE[DIRECTION_COUNT][4] = {
    [DIR_POS_X] = {
        {1.0f, 1.0f, 0.0f}, 
        {1.0f, 0.0f, 0.0f}, 
        {1.0f, 0.0f, 1.0f}, 
        {1.0f, 1.0f, 1.0f}, 
    },
    [DIR_POS_Y] = {
        {0.0f, 1.0f, 1.0f}, 
        {0.0f, 1.0f, 0.0f}, 
        {1.0f, 1.0f, 0.0f}, 
        {1.0f, 1.0f, 1.0f}, 
    },
    [DIR_POS_Z] = {
        {1.0f, 1.0f, 1.0f},
        {1.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 1.0f},
    },
    [DIR_NEG_X] = {
        {0.0f, 1.0f, 1.0f}, 
        {0.0f, 0.0f, 1.0f}, 
        {0.0f, 0.0f, 0.0f}, 
        {0.0f, 1.0f, 0.0f}, 
    },
    [DIR_NEG_Y] = {
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f},
        {1.0f, 0.0f, 1.0f},
        {1.0f, 0.0f, 0.0f},
    },
    [DIR_NEG_Z] = {
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 0.0f}, 
    },
};
/* clang-format on */

Block_Vertex *vertices = NULL;
size_t vertex_count = 0;

static void mesh_block(const Chunk *chunk, iVec3 pos) {
    Block_Type current_block = chunk->blocks[chunk_index(pos)];
    if (current_block == BLOCK_AIR) {
        return;
    }

    for (Direction dir = 0; dir < DIRECTION_COUNT; dir++) {
        iVec3 normal = direction_to_ivec3(dir);
        Block_Type neighbor = chunk_get_block(chunk, ivec3_add(pos, normal));
        const Block_Properties *neighbor_properties = get_block_properties(neighbor);

        Vec3 fpos = {pos.x, pos.y, pos.z};

        if (neighbor_properties->is_transparent) {
            vertices[vertex_count++] = (Block_Vertex){
                .position = vec3_add(fpos, FACE_VERTEX_TABLE[dir][0]),
            };

            vertices[vertex_count++] = (Block_Vertex){
                .position = vec3_add(fpos, FACE_VERTEX_TABLE[dir][1]),
            };

            vertices[vertex_count++] = (Block_Vertex){
                .position = vec3_add(fpos, FACE_VERTEX_TABLE[dir][2]),
            };

            vertices[vertex_count++] = (Block_Vertex){
                .position = vec3_add(fpos, FACE_VERTEX_TABLE[dir][3]),
            };
        }
    }
}

static void mesh_chunk(const Chunk *chunk) {
    if (!vertices) {
        vertices = malloc(sizeof(Block_Vertex) * MAX_VERTICES);
    }

    vertex_count = 0;

    for (int z = 0; z < CHUNK_SIZE_Z; z++) {
        for (int y = 0; y < CHUNK_SIZE_Y; y++) {
            for (int x = 0; x < CHUNK_SIZE_X; x++) {
                mesh_block(chunk, (iVec3){x, y, z});
            }
        }
    }
}

int main(void) {
    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        fprintf(stderr, "glfwInit() failed\n");
        return EXIT_FAILURE;
    }

    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);

    glfwWindowHint(GLFW_RED_BITS, mode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow *window = glfwCreateWindow(mode->width, mode->height, "Quadcraft", monitor, NULL);
    if (!window) {
        fprintf(stderr, "glfwCreateWindow() failed\n");
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGL(glfwGetProcAddress)) {
        fprintf(stderr, "gladLoadGL() failed\n");
        return EXIT_FAILURE;
    }

    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, cursor_callback);

    Chunk chunk = {0};
    for (int z = 0; z < CHUNK_SIZE_Z; z++) {
        for (int y = 0; y < CHUNK_SIZE_Y; y++) {
            for (int x = 0; x < CHUNK_SIZE_X; x++) {
                chunk.blocks[chunk_index((iVec3){x, y, z})] = BLOCK_DIRT;
            }
        }
    }

    mesh_chunk(&chunk);

    uint32_t *indices = malloc(sizeof(uint32_t) * MAX_INDICES);
    uint32_t offset = 0;
    for (uint32_t i = 0; i < MAX_INDICES; i += 6) {
        indices[i + 0] = offset + 3;
        indices[i + 1] = offset + 2;
        indices[i + 2] = offset + 1;
        indices[i + 3] = offset + 3;
        indices[i + 4] = offset + 1;
        indices[i + 5] = offset + 0;

        offset += 4;
    }

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Block_Vertex) * vertex_count, vertices, GL_STATIC_DRAW);

    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * MAX_INDICES, indices, GL_STATIC_DRAW);

    /* Position attribute */
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Block_Vertex),
                          (void *)offsetof(Block_Vertex, position));

    GLuint program = compile_program_from_files("res/shaders/chunk.vert", "res/shaders/chunk.frag");
    if (!program) {
        fprintf(stderr, "compile_program_from_files failed\n");
        return EXIT_FAILURE;
    }

    camera = (Camera){
        .position = {0.0f, 0.0f, 0.0f},
        .pitch = 0.0f,
        .yaw = -HALF_PI,
        .fov = to_radians(90.0f),
        .aspect = mode->width / (float)mode->height,
        .near = 0.01f,
        .far = 1000.0f,
    };

    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    float current_time = (float)glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        float new_time = (float)glfwGetTime();
        float delta_time = new_time - current_time;
        current_time = new_time;

        Vec3 move_dir = {0};
        if (glfwGetKey(window, GLFW_KEY_W)) {
            move_dir = vec3_add(move_dir, camera.forward);
        }

        if (glfwGetKey(window, GLFW_KEY_S)) {
            move_dir = vec3_sub(move_dir, camera.forward);
        }

        if (glfwGetKey(window, GLFW_KEY_D)) {
            move_dir = vec3_add(move_dir, camera.right);
        }

        if (glfwGetKey(window, GLFW_KEY_A)) {
            move_dir = vec3_sub(move_dir, camera.right);
        }

        camera.position = vec3_add(camera.position,
                                   vec3_scale(vec3_normalize(move_dir), delta_time * CAMERA_SPEED));

        camera_update(&camera);

        glClearColor(0.39, 0.58, 0.93, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);
        glUniformMatrix4fv(glGetUniformLocation(program, "u_view"), 1, GL_FALSE, camera.view.data);
        glUniformMatrix4fv(glGetUniformLocation(program, "u_proj"), 1, GL_FALSE, camera.proj.data);

        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, (vertex_count / 4) * 6, GL_UNSIGNED_INT, NULL);

        glUseProgram(0);

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}
