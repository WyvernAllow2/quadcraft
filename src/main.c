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
#include "world.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <stb_image.h>

#define CAMERA_SPEED 5.0f
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

    Mesh_Allocator allocator;
    World *world;
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

static bool is_block_transparent(Block_Type type) {
    return get_block_properties(type)->is_transparent;
}

static bool chunk_is_block_in_local_bounds(iVec3 pos) {
    return pos.x >= 0 && pos.x < CHUNK_SIZE && pos.y >= 0 && pos.y < CHUNK_SIZE && pos.z >= 0 &&
           pos.z < CHUNK_SIZE;
}

static bool is_transparent_neighbor(const Chunk *chunk, const Chunk *neighbors[DIRECTION_COUNT],
                                    iVec3 pos, Direction dir) {
    iVec3 normal = direction_to_ivec3(dir);
    iVec3 neighbor_pos = ivec3_add(pos, normal);

    if (chunk_is_block_in_local_bounds(neighbor_pos)) {
        return is_block_transparent(chunk_get_block_unsafe(chunk, neighbor_pos));
    }

    const Chunk *neighbor_chunk = neighbors[dir];
    if (!neighbor_chunk) {
        return true;
    }

    iVec3 neighbor_local = {
        .x = mod(neighbor_pos.x, CHUNK_SIZE),
        .y = mod(neighbor_pos.y, CHUNK_SIZE),
        .z = mod(neighbor_pos.z, CHUNK_SIZE),
    };

    return is_block_transparent(chunk_get_block_unsafe(neighbor_chunk, neighbor_local));
}

static void mesh_block(const Chunk *chunk, const Chunk *neighbors[DIRECTION_COUNT], Block_Type type,
                       iVec3 local_position) {
    const Block_Properties *properties = get_block_properties(type);

    for (Direction dir = 0; dir < DIRECTION_COUNT; dir++) {
        if (is_transparent_neighbor(chunk, neighbors, local_position, dir)) {
            emit_face(local_position, dir, properties->face_textures[dir]);
        }
    }
}

static void mesh_chunk(iVec3 chunk_coord) {
    state.vertex_count = 0;

    Chunk *chunk = world_get_chunk_unsafe(state.world, chunk_coord);
    const Chunk *neighbors[DIRECTION_COUNT];
    for (Direction direction = 0; direction < DIRECTION_COUNT; direction++) {
        iVec3 normal = direction_to_ivec3(direction);
        iVec3 neighbor_pos = ivec3_add(chunk_coord, normal);
        neighbors[direction] = world_get_chunk(state.world, neighbor_pos);
    }

    for (int z = 0; z < CHUNK_SIZE; z++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int x = 0; x < CHUNK_SIZE; x++) {
                iVec3 coord = {x, y, z};
                Block_Type type = chunk_get_block_unsafe(chunk, coord);

                if (type != BLOCK_AIR) {
                    mesh_block(chunk, neighbors, type, coord);
                }
            }
        }
    }
}

static void generate_chunk(Chunk *chunk, iVec3 chunk_coord) {
    chunk->is_dirty = false;
    chunk->mesh.length = 0;
    chunk->mesh.offset = 0;
    chunk->coord = chunk_coord;

    iVec3 chunk_offset = {
        .x = chunk_coord.x * CHUNK_SIZE,
        .y = chunk_coord.y * CHUNK_SIZE,
        .z = chunk_coord.z * CHUNK_SIZE,
    };

    for (int z = 0; z < CHUNK_SIZE; z++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int x = 0; x < CHUNK_SIZE; x++) {
                iVec3 local_pos = {x, y, z};
                iVec3 block_position = ivec3_add(chunk_offset, local_pos);

                int height = 100;
                if (block_position.y > height) {
                    chunk_set_block_unsafe(chunk, local_pos, BLOCK_AIR);
                } else if (block_position.y == height) {
                    chunk_set_block_unsafe(chunk, local_pos, BLOCK_GRASS);
                } else {
                    chunk_set_block_unsafe(chunk, local_pos, BLOCK_DIRT);
                }
            }
        }
    }
}

static void generate_world(World *world) {
    for (int z = 0; z < WORLD_SIZE_Z; z++) {
        for (int y = 0; y < WORLD_SIZE_Y; y++) {
            for (int x = 0; x < WORLD_SIZE_X; x++) {
                iVec3 coord = {x, y, z};
                Chunk *chunk = world_get_chunk_unsafe(world, coord);
                generate_chunk(chunk, coord);
                world_mark_chunk_dirty(world, chunk);
            }
        }
    }
}

static Chunk *pop_next_dirty(iVec3 player_coord) {
    if (state.world->dirty_chunk_count == 0) {
        return NULL;
    }

    int min_distance_squared = INT_MAX;
    Chunk *closest_dirty = NULL;
    size_t closest_dirty_index = 0;

    for (size_t i = 0; i < state.world->dirty_chunk_count; i++) {
        Chunk *chunk = state.world->dirty_chunks[i];
        iVec3 delta = ivec3_sub(player_coord, chunk->coord);
        int distance_squared = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;

        if (distance_squared < min_distance_squared) {
            min_distance_squared = distance_squared;
            closest_dirty = chunk;
            closest_dirty_index = i;
        }
    }

    state.world->dirty_chunks[closest_dirty_index] =
        state.world->dirty_chunks[state.world->dirty_chunk_count - 1];
    state.world->dirty_chunk_count--;

    closest_dirty->is_dirty = false;
    return closest_dirty;
}

Vec3 cam_forward;

Block_Type place_block = 1;

static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
    (void)window;
    (void)button;
    (void)mods;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        Hit_Result result = world_raycast(state.world, state.camera.position, cam_forward);
        if (result.did_hit) {
            world_set_block(state.world, result.position, BLOCK_AIR);
        }
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        Hit_Result result = world_raycast(state.world, state.camera.position, cam_forward);
        if (result.did_hit) {
            world_set_block(state.world, ivec3_add(result.position, result.normal), place_block);
        }
    }
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    (void)window;
    (void)scancode;
    (void)mods;

    if (key >= GLFW_KEY_0 && key < GLFW_KEY_9 && action == GLFW_PRESS) {
        place_block = (key - GLFW_KEY_0) + 1;
        if (place_block > BLOCK_TYPE_COUNT) {
            place_block = 1;
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
    glfwWindowHint(GLFW_SAMPLES, 4);
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
    glfwSetMouseButtonCallback(state.window, mouse_button_callback);

    state.first_mouse = true;
    glfwSetInputMode(state.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(state.window, cursor_pos_callback);
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(state.window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    glfwSetKeyCallback(state.window, key_callback);

    state.shader = compile_program_from_files("res/shaders/chunk.vert", "res/shaders/chunk.frag");
    if (!state.shader) {
        fprintf(stderr, "compile_program_from_files() failed\n");
        return EXIT_FAILURE;
    }

    state.world = malloc(sizeof(World));
    state.vertices = malloc(sizeof(Vertex) * MAX_VERTS);
    state.vertex_count = 0;

    mesh_allocator_init(&state.allocator, MAX_QUADS * 25);

    state.camera = (Camera){
        .position = {0, 120, 0},
        .pitch = 0.0f,
        .yaw = -to_radians(90.0f),
        .fov = to_radians(90.0f),
        .aspect = mode->width / (float)mode->height,
        .znear = 0.1f,
        .zfar = 1000.0f,
    };

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glfwSwapInterval(1);

    GLuint textures = load_textures();

    generate_world(state.world);

    float old_time = glfwGetTime();
    while (!glfwWindowShouldClose(state.window)) {
        glfwPollEvents();

        float new_time = glfwGetTime();
        float delta_time = new_time - old_time;
        old_time = new_time;

        cam_forward = vec3_normalize((Vec3){
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

        state.camera.position = vec3_add(
            state.camera.position, vec3_scale(vec3_normalize(wish_dir), CAMERA_SPEED * delta_time));

        iVec3 player_coord = {
            .x = state.camera.position.x / CHUNK_SIZE,
            .y = state.camera.position.y / CHUNK_SIZE,
            .z = state.camera.position.z / CHUNK_SIZE,
        };

        Chunk *next = pop_next_dirty(player_coord);
        if (next) {
            mesh_chunk(next->coord);
            mesh_allocator_upload(&state.allocator, &next->mesh, state.vertices,
                                  state.vertex_count);
        }

        Mat4 view;
        mat4_look_at(&view, state.camera.position, vec3_add(state.camera.position, cam_forward),
                     cam_up);
        Mat4 proj;
        mat4_perspective(&proj, state.camera.fov, state.camera.aspect, state.camera.znear,
                         state.camera.zfar);

        glClearColor(0.7f, 0.7f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(state.allocator.vao);
        glUseProgram(state.shader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, textures);
        uniform_int(state.shader, "u_textures", 0);

        uniform_mat4(state.shader, "u_view", &view);
        uniform_mat4(state.shader, "u_proj", &proj);

        for (size_t i = 0; i < WORLD_VOLUME; i++) {
            Chunk *chunk = &state.world->chunk[i];

            if (chunk->mesh.length > 0) {
                glUniform3i(glGetUniformLocation(state.shader, "u_chunk_position"), chunk->coord.x,
                            chunk->coord.y, chunk->coord.z);

                glDrawElementsBaseVertex(GL_TRIANGLES, (chunk->mesh.length / 4) * 6,
                                         GL_UNSIGNED_INT, NULL, (int)chunk->mesh.offset);
            }
        }

        glUseProgram(0);
        glBindVertexArray(0);

        glfwSwapBuffers(state.window);
    }

    glDeleteProgram(state.shader);

    glfwDestroyWindow(state.window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
