#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "direction.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <stb_image.h>

#include "block_type.h"
#include "camera.h"
#include "chunk.h"
#include "math3d.h"
#include "mesh_allocator.h"
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

Block_Type place_block = BLOCK_DIRT;

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    (void)scancode;
    (void)mods;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
        place_block = key - GLFW_KEY_0;
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

    const Block_Properties *properties = get_block_properties(current_block);

    for (Direction dir = 0; dir < DIRECTION_COUNT; dir++) {
        iVec3 normal = direction_to_ivec3(dir);
        Vec3 fnormal = direction_to_vec3(dir);
        Block_Type neighbor = chunk_get_block(chunk, ivec3_add(pos, normal));
        const Block_Properties *neighbor_properties = get_block_properties(neighbor);

        Vec3 fpos = {pos.x, pos.y, pos.z};

        if (neighbor_properties->is_transparent) {
            vertices[vertex_count++] = (Block_Vertex){
                .position = vec3_add(fpos, FACE_VERTEX_TABLE[dir][0]),
                .normal = fnormal,
                .layer = properties->face_textures[dir],
            };

            vertices[vertex_count++] = (Block_Vertex){
                .position = vec3_add(fpos, FACE_VERTEX_TABLE[dir][1]),
                .normal = fnormal,
                .layer = properties->face_textures[dir],
            };

            vertices[vertex_count++] = (Block_Vertex){
                .position = vec3_add(fpos, FACE_VERTEX_TABLE[dir][2]),
                .normal = fnormal,
                .layer = properties->face_textures[dir],
            };

            vertices[vertex_count++] = (Block_Vertex){
                .position = vec3_add(fpos, FACE_VERTEX_TABLE[dir][3]),
                .normal = fnormal,
                .layer = properties->face_textures[dir],
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

static GLuint load_texture_array(void) {
    GLuint texture_array;
    glGenTextures(1, &texture_array);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture_array);

    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, TEXTURE_WIDTH, TEXTURE_HEIGHT, TEXTURE_ID_COUNT,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_set_flip_vertically_on_load(true);

    for (int i = 0; i < TEXTURE_ID_COUNT; ++i) {
        int width, height, channels;
        const char *path = get_texture_path((Texture_ID)i);

        unsigned char *data = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);
        if (!data) {
            fprintf(stderr, "Failed to load texture: %s\n", path);
            continue;
        }

        if (width != TEXTURE_WIDTH || height != TEXTURE_HEIGHT) {
            fprintf(stderr, "Texture %s has incorrect size (%dx%d), expected %dx%d\n", path, width,
                    height, TEXTURE_WIDTH, TEXTURE_HEIGHT);
            stbi_image_free(data);
            continue;
        }

        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, TEXTURE_WIDTH, TEXTURE_HEIGHT, 1, GL_RGBA,
                        GL_UNSIGNED_BYTE, data);

        stbi_image_free(data);
    }

    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    return texture_array;
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

    Chunk *chunk = malloc(sizeof(Chunk));
    chunk->mesh.vertex_count = 0;
    chunk->mesh.start = 0;
    memset(chunk->blocks, 0, CHUNK_VOLUME);

    for (int z = 0; z < CHUNK_SIZE_Z; z++) {
        for (int y = 0; y < CHUNK_SIZE_Y; y++) {
            for (int x = 0; x < CHUNK_SIZE_X; x++) {
                if (y < 8) {
                    chunk->blocks[chunk_index((iVec3){x, y, z})] = BLOCK_DIRT;
                } else if (y == 8) {
                    chunk->blocks[chunk_index((iVec3){x, y, z})] = BLOCK_GRASS;
                }
            }
        }
    }

    chunk->dirty = true;

    Mesh_Allocator allocator;
    mesh_allocator_init(&allocator, MAX_VERTICES * 5);

    GLuint program = compile_program_from_files("res/shaders/chunk.vert", "res/shaders/chunk.frag");
    if (!program) {
        fprintf(stderr, "compile_program_from_files failed\n");
        return EXIT_FAILURE;
    }

    GLuint texture_array = load_texture_array();

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

        move_dir = vec3_normalize(move_dir);

        camera.position =
            vec3_add(camera.position, vec3_scale(move_dir, delta_time * CAMERA_SPEED));

        camera_update(&camera);

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1)) {
            int x = floorf(camera.position.x + camera.forward.x);
            int y = floorf(camera.position.y + camera.forward.y);
            int z = floorf(camera.position.z + camera.forward.z);
            iVec3 pos = {x, y, z};
            chunk_set_block(chunk, pos, BLOCK_AIR);
        }

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_2)) {
            int x = floorf(camera.position.x + camera.forward.x);
            int y = floorf(camera.position.y + camera.forward.y);
            int z = floorf(camera.position.z + camera.forward.z);
            iVec3 pos = {x, y, z};
            chunk_set_block(chunk, pos, place_block);
        }

        if (chunk->dirty) {
            mesh_chunk(chunk);
            upload_mesh(&allocator, &chunk->mesh, vertices, vertex_count);

            fprintf(stderr, "%zu, %zu\n", chunk->mesh.start, chunk->mesh.vertex_count);
        }

        glClearColor(0.39, 0.58, 0.93, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);
        glUniformMatrix4fv(glGetUniformLocation(program, "u_view"), 1, GL_FALSE, camera.view.data);
        glUniformMatrix4fv(glGetUniformLocation(program, "u_proj"), 1, GL_FALSE, camera.proj.data);
        glUniform3f(glGetUniformLocation(program, "u_camera_position"), camera.position.x,
                    camera.position.y, camera.position.z);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, texture_array);
        glUniform1i(glGetUniformLocation(program, "u_texture_array"), 0);

        glBindVertexArray(allocator.vao);

        glDrawElementsBaseVertex(GL_TRIANGLES, (vertex_count / 4) * 6, GL_UNSIGNED_INT, NULL,
                                 (int)chunk->mesh.start);

        glUseProgram(0);

        glfwSwapBuffers(window);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}
