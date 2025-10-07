#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "blocks.h"
#include "direction.h"
#include "math3d.h"
#include "mesh_allocator.h"
#include "texture_id.h"
#include "utils.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <stb_image.h>

#define CAMERA_SPEED 4.0f
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

struct {
    int window_w;
    int window_h;

    GLFWwindow *window;

    GLuint shader;

    Camera camera;
    float old_mouse_x;
    float old_mouse_y;
    bool first_mouse;

    Vertex *vertices;
    size_t vertex_count;

    Mesh_Allocator meshes;
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

static void uniform_int(GLuint program, const char *name, int value) {
    glUniform1i(glGetUniformLocation(program, name), value);
}

#define CHUNK_SIZE 32
#define CHUNK_VOLUME (CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE)

typedef struct Chunk {
    uint8_t blocks[CHUNK_VOLUME];
    bool is_dirty;
} Chunk;

static bool in_chunk_bounds(iVec3 local_coords) {
    return local_coords.x >= 0 && local_coords.y >= 0 && local_coords.z >= 0 &&
           local_coords.x < CHUNK_SIZE && local_coords.y < CHUNK_SIZE &&
           local_coords.z < CHUNK_SIZE;
}

static int get_block_index(iVec3 local_coords) {
    int x = local_coords.x;
    int y = local_coords.y;
    int z = local_coords.z;
    return x + CHUNK_SIZE * (y + CHUNK_SIZE * z);
}

static Block_Type get_block(const Chunk *chunk, iVec3 coord) {
    if (!in_chunk_bounds(coord)) {
        return BLOCK_AIR;
    }

    return chunk->blocks[get_block_index(coord)];
}

static void set_block(Chunk *chunk, iVec3 coord, Block_Type type) {
    if (!in_chunk_bounds(coord)) {
        return;
    }

    chunk->blocks[get_block_index(coord)] = type;
}

static bool is_transparent(const Chunk *chunk, iVec3 coord) {
    Block_Type block = get_block(chunk, coord);
    return get_block_properties(block)->is_transparent;
}

static GLuint load_textures(void) {
    stbi_set_flip_vertically_on_load(true);

    GLuint texture_array;
    glGenTextures(1, &texture_array);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture_array);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, TEXTURE_SIZE, TEXTURE_SIZE, TEXTURE_ID_COUNT);

    for (int i = 0; i < TEXTURE_ID_COUNT; ++i) {
        const char *filename = get_texture_filename(i);

        int width;
        int height;
        int num_channels;
        uint8_t *data = stbi_load(filename, &width, &height, &num_channels, STBI_rgb_alpha);
        if (!data) {
            fprintf(stderr, "Failed to load texture: %s\n", get_texture_filename(i));
            continue;
        }

        if (width != 16 || height != 16) {
            fprintf(stderr, "Invalid texture size: %s\n", get_texture_filename(i));
            continue;
        }

        glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                        0,                 // mip level
                        0, 0, i,           // x, y, layer
                        width, height, 1,  // width, height, depth
                        GL_RGBA, GL_UNSIGNED_BYTE, data);

        stbi_image_free(data);
    }

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    return texture_array;
}

/* The maximum number of quads a chunk could possibly have. Assuming the worse-case scenario of a 3D
   checkerboard pattern, then half the blocks would have all 6 faces exposed.
*/
#define MAX_QUADS ((CHUNK_VOLUME / 2) * 6)
#define MAX_VERTS (MAX_QUADS * 4)
#define MAX_INDICES (MAX_QUADS * 6)

/* clang-format off */
static const iVec3 FACE_VERTICES[DIRECTION_COUNT][4] = {
    [DIR_POSITIVE_X] = {
        {1, 0, 0},
        {1, 1, 0},
        {1, 1, 1},
        {1, 0, 1},
    },
    [DIR_POSITIVE_Y] = {
        {1, 1, 1},
        {1, 1, 0},
        {0, 1, 0},
        {0, 1, 1},
    },
    [DIR_POSITIVE_Z] = {
        {0, 0, 1},
        {1, 0, 1},
        {1, 1, 1},
        {0, 1, 1},
    },
    [DIR_NEGATIVE_X] = {
        {0, 0, 1},
        {0, 1, 1},
        {0, 1, 0},
        {0, 0, 0},
    },
    [DIR_NEGATIVE_Y] = {
        {0, 0, 1},
        {0, 0, 0},
        {1, 0, 0},
        {1, 0, 1},
    },
    [DIR_NEGATIVE_Z] = {
        {0, 1, 0},
        {1, 1, 0},
        {1, 0, 0},
        {0, 0, 0},
    },
};
/* clang-format on */

static void emit_face(iVec3 coord, Direction direction, Texture_ID texture) {
    iVec3 normal = direction_to_ivec3(direction);

    for (size_t i = 0; i < 4; i++) {
        state.vertices[state.vertex_count + i] = (Vertex){
            .position = vec3_from_ivec3(ivec3_add(coord, FACE_VERTICES[direction][i])),
            .normal = vec3_from_ivec3(normal),
            .texture = texture,
        };
    }

    state.vertex_count += 4;
}

static void mesh_block(const Chunk *chunk, iVec3 coord) {
    const Block_Properties *properties = get_block_properties(get_block(chunk, coord));

    for (Direction direction = 0; direction < DIRECTION_COUNT; direction++) {
        iVec3 normal = direction_to_ivec3(direction);

        if (is_transparent(chunk, ivec3_add(coord, normal))) {
            emit_face(coord, direction, properties->face_textures[direction]);
        }
    }
}

static void mesh_chunk(const Chunk *chunk) {
    state.vertex_count = 0;

    for (int z = 0; z < CHUNK_SIZE; z++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int x = 0; x < CHUNK_SIZE; x++) {
                iVec3 coord = {x, y, z};
                int index = get_block_index(coord);

                if (chunk->blocks[index] != BLOCK_AIR) {
                    mesh_block(chunk, coord);
                }
            }
        }
    }
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

    Chunk chunk;
    for (int z = 0; z < CHUNK_SIZE; z++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int x = 0; x < CHUNK_SIZE; x++) {
                int index = get_block_index((iVec3){x, y, z});
                chunk.blocks[index] = BLOCK_DIRT;
            }
        }
    }

    chunk.is_dirty = true;

    state.vertices = malloc(sizeof(Vertex) * MAX_VERTS);
    state.vertex_count = 0;

    // uint32_t *indices = malloc(sizeof(uint32_t) * MAX_INDICES);
    // for (uint32_t i = 0; i < MAX_QUADS; i++) {
    //     indices[i * 6 + 0] = 0 + i * 4;
    //     indices[i * 6 + 1] = 1 + i * 4;
    //     indices[i * 6 + 2] = 3 + i * 4;
    //     indices[i * 6 + 3] = 1 + i * 4;
    //     indices[i * 6 + 4] = 2 + i * 4;
    //     indices[i * 6 + 5] = 3 + i * 4;
    // }

    // glGenVertexArrays(1, &state.vao);
    // glGenBuffers(1, &state.vbo);
    // glGenBuffers(1, &state.ebo);

    // glBindVertexArray(state.vao);

    // glBindBuffer(GL_ARRAY_BUFFER, state.vbo);
    // glBufferData(GL_ARRAY_BUFFER, MAX_VERTS * sizeof(Vertex), NULL, GL_DYNAMIC_DRAW);

    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state.ebo);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_INDICES * sizeof(uint32_t), indices,
    // GL_STATIC_DRAW);

    // free(indices);

    // const void *position_offset = (const void *)offsetof(Vertex, position);
    // const void *normal_offset = (const void *)offsetof(Vertex, normal);

    // /* Position attribute */
    // glEnableVertexAttribArray(0);
    // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), position_offset);

    // /* Normal attribute */
    // glEnableVertexAttribArray(1);
    // glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), normal_offset);

    // glBindVertexArray(0);

    mesh_allocator_init(&state.meshes, MAX_QUADS * 4);

    state.camera = (Camera){
        .position = {0, 0, 0},
        .pitch = 0.0f,
        .yaw = -to_radians(90.0f),
        .fov = to_radians(90.0f),
        .aspect = mode->width / (float)mode->height,
        .znear = 0.1f,
        .zfar = 500.0f,
    };

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    Mesh mesh = {0};

    GLuint textures = load_textures();

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

        if (glfwGetKey(state.window, GLFW_KEY_SPACE)) {
            wish_dir.y += 1;
        }

        if (glfwGetKey(state.window, GLFW_KEY_LEFT_SHIFT)) {
            wish_dir.y -= 1;
        }

        if (glfwGetMouseButton(state.window, GLFW_MOUSE_BUTTON_LEFT)) {
            iVec3 coord = {
                state.camera.position.x + cam_forward.x,
                state.camera.position.y + cam_forward.y,
                state.camera.position.z + cam_forward.z,
            };

            if (get_block(&chunk, coord) != BLOCK_AIR) {
                set_block(&chunk, coord, BLOCK_AIR);
                chunk.is_dirty = true;
            }
        }

        if (glfwGetMouseButton(state.window, GLFW_MOUSE_BUTTON_RIGHT)) {
            iVec3 coord = {
                state.camera.position.x + cam_forward.x,
                state.camera.position.y + cam_forward.y,
                state.camera.position.z + cam_forward.z,
            };

            if (get_block(&chunk, coord) != BLOCK_DEBUG) {
                set_block(&chunk, coord, BLOCK_DEBUG);
                chunk.is_dirty = true;
            }
        }

        state.camera.position = vec3_add(
            state.camera.position, vec3_scale(vec3_normalize(wish_dir), CAMERA_SPEED * delta_time));

        if (chunk.is_dirty) {
            mesh_chunk(&chunk);
            mesh_allocator_upload(&state.meshes, &mesh, state.vertices, state.vertex_count);

            printf("Mesh allocated: {offset: %zu, length: %zu}\n", mesh.offset, mesh.length);

            chunk.is_dirty = false;
        }

        Mat4 view;
        mat4_look_at(&view, state.camera.position, vec3_add(state.camera.position, cam_forward),
                     cam_up);
        Mat4 proj;
        mat4_perspective(&proj, state.camera.fov, state.camera.aspect, state.camera.znear,
                         state.camera.zfar);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(state.meshes.vao);
        glUseProgram(state.shader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, textures);
        uniform_int(state.shader, "u_textures", 0);

        uniform_mat4(state.shader, "u_view", &view);
        uniform_mat4(state.shader, "u_proj", &proj);

        glDrawElementsBaseVertex(GL_TRIANGLES, (mesh.length / 4) * 6, GL_UNSIGNED_INT, NULL,
                                 mesh.offset);

        glUseProgram(0);
        glBindVertexArray(0);

        glfwSwapBuffers(state.window);
    }

    glDeleteProgram(state.shader);

    glfwDestroyWindow(state.window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
