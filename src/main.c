#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "blocks.h"
#include "camera.h"
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

#define CAMERA_SPEED 10.0f
#define MOUSE_SENSITIVITY 0.125f

typedef struct AABB {
    Vec3 position;
    Vec3 size;
} AABB;

static bool aabb_vs_aabb(const AABB *a, const AABB *b) {
    Vec3 a_min_extent = a->position;
    Vec3 a_max_extent = vec3_add(a->position, a->size);

    Vec3 b_min_extent = b->position;
    Vec3 b_max_extent = vec3_add(b->position, b->size);

    return (a_min_extent.x < b_max_extent.x && a_max_extent.x > b_min_extent.x) &&
           (a_min_extent.y < b_max_extent.y && a_max_extent.y > b_min_extent.y) &&
           (a_min_extent.z < b_max_extent.z && a_max_extent.z > b_min_extent.z);
}

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

    AABB player_aabb;
    Vec3 player_velocity;
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

Block_Type place_block = 1;
Hit_Result result;

static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
    (void)window;
    (void)button;
    (void)mods;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        if (result.did_hit) {
            world_set_block(state.world, result.position, BLOCK_AIR);
        }
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        if (result.did_hit) {
            iVec3 place_pos = ivec3_add(result.position, result.normal);

            AABB resultant_aabb = {vec3_from_ivec3(place_pos), .size = {1, 1, 1}};
            if (!aabb_vs_aabb(&state.player_aabb, &resultant_aabb)) {
                world_set_block(state.world, place_pos, place_block);
            }
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

typedef struct DebugVertex {
    Vec3 position;
    Vec3 color;
} DebugVertex;

#define MAX_LINES 10000
int debug_vertex_count = 0;
DebugVertex *debug_vertices;

static void push_line(Vec3 start, Vec3 end, Vec3 color) {
    if (debug_vertex_count + 2 > MAX_LINES) {
        return;
    }

    debug_vertices[debug_vertex_count++] = (DebugVertex){
        .position = start,
        .color = color,
    };

    debug_vertices[debug_vertex_count++] = (DebugVertex){
        .position = end,
        .color = color,
    };
}

static void push_cube(Vec3 position, Vec3 size, Vec3 color) {
    Vec3 max = vec3_add(position, size);

    Vec3 p[8] = {
        {position.x, position.y, position.z},
        {max.x, position.y, position.z},
        {max.x, max.y, position.z},
        {position.x, max.y, position.z},
        {position.x, position.y, max.z},
        {max.x, position.y, max.z},
        {max.x, max.y, max.z},
        {position.x, max.y, max.z},
    };

    push_line(p[0], p[1], color);
    push_line(p[1], p[2], color);
    push_line(p[2], p[3], color);
    push_line(p[3], p[0], color);

    push_line(p[4], p[5], color);
    push_line(p[5], p[6], color);
    push_line(p[6], p[7], color);
    push_line(p[7], p[4], color);

    push_line(p[0], p[4], color);
    push_line(p[1], p[5], color);
    push_line(p[2], p[6], color);
    push_line(p[3], p[7], color);
}

typedef struct Collision_Result {
    bool did_collide;
    float entry_time;
    Vec3 normal;
} Collision_Result;

static Collision_Result no_collision(void) {
    return (Collision_Result){
        .did_collide = false,
        .entry_time = 1.0f,
        .normal = {0.0f, 0.0f, 0.0f},
    };
}

static float remap_time(float inv_entry, float velocity) {
    if (velocity == 0.0f) {
        return (inv_entry > 0) ? -INFINITY : INFINITY;
    } else {
        return inv_entry / velocity;
    }
}

static Collision_Result aabb_collide(const AABB *dyn, const AABB *stat, Vec3 velocity) {
    Vec3 dyn_min = dyn->position;
    Vec3 dyn_max = vec3_add(dyn->position, dyn->size);

    Vec3 stat_min = stat->position;
    Vec3 stat_max = vec3_add(stat->position, stat->size);

    float vx = velocity.x;
    float vy = velocity.y;
    float vz = velocity.z;

    float inv_x_entry = (vx > 0.0f) ? stat_min.x - dyn_max.x : stat_max.x - dyn_min.x;
    float inv_x_exit = (vx > 0.0f) ? stat_max.x - dyn_min.x : stat_min.x - dyn_max.x;

    float inv_y_entry = (vy > 0.0f) ? stat_min.y - dyn_max.y : stat_max.y - dyn_min.y;
    float inv_y_exit = (vy > 0.0f) ? stat_max.y - dyn_min.y : stat_min.y - dyn_max.y;

    float inv_z_entry = (vz > 0.0f) ? stat_min.z - dyn_max.z : stat_max.z - dyn_min.z;
    float inv_z_exit = (vz > 0.0f) ? stat_max.z - dyn_min.z : stat_min.z - dyn_max.z;

    /* clang-format off */
    float x_entry = remap_time(inv_x_entry, velocity.x);
    float x_exit =  remap_time(inv_x_exit, velocity.x);

    float y_entry = remap_time(inv_y_entry, velocity.y);
    float y_exit =  remap_time(inv_y_exit, velocity.y);

    float z_entry = remap_time(inv_z_entry, velocity.z);
    float z_exit =  remap_time(inv_z_exit, velocity.z);
    /* clang-format on */

    if (x_entry < 0.0f && y_entry < 0.0f && z_entry < 0.0f) {
        return no_collision();
    }

    if (x_entry > 1.0f || y_entry > 1.0f || z_entry > 1.0f) {
        return no_collision();
    }

    float entry = fmaxf(x_entry, fmax(y_entry, z_entry));
    float exit = fminf(x_exit, fminf(y_exit, z_exit));

    if (entry > exit) {
        return no_collision();
    }

    Vec3 normal = {0};
    if (entry == x_entry) {
        normal.x = (vx > 0) ? -1 : 1;
    }

    if (entry == y_entry) {
        normal.y = (vy > 0) ? -1 : 1;
    }

    if (entry == z_entry) {
        normal.z = (vz > 0) ? -1 : 1;
    }

    return (Collision_Result){
        .entry_time = entry,
        .normal = normal,
        .did_collide = true,
    };
}

bool on_ground = false;

static void update_collision(AABB *e, Vec3 *velocity, float delta_time) {
    on_ground = false;
    for (int step = 0; step < 3; step++) {
        Vec3 adjusted_velocity = {
            velocity->x * delta_time,
            velocity->y * delta_time,
            velocity->z * delta_time,
        };

        int size = 3;

        int start_x = e->position.x - size;
        int start_y = e->position.y - size;
        int start_z = e->position.z - size;

        const int MAX_COLLISIONS = 100;
        Collision_Result potential_collisions[MAX_COLLISIONS];
        int collision_count = 0;

        for (int ix = start_x; ix < start_x + size * 2; ix++) {
            for (int iy = start_y; iy < start_y + size * 2; iy++) {
                for (int iz = start_z; iz < start_z + size * 2; iz++) {
                    iVec3 posv = {ix, iy, iz};

                    Block_Type type = world_get_block(state.world, posv);

                    if (type == BLOCK_AIR) {
                        continue;
                    }

                    AABB block_collider = {
                        .position = (Vec3){ix, iy, iz},
                        .size = (Vec3){1, 1, 1},
                    };

                    Collision_Result collision =
                        aabb_collide(e, &block_collider, adjusted_velocity);
                    if (collision.did_collide) {
                        if (collision_count >= MAX_COLLISIONS) {
                            fprintf(stderr, "Too many colliders\n");
                            break;
                        }

                        potential_collisions[collision_count++] = collision;
                    }
                }
            }
        }

        if (collision_count == 0) {
            break;
        }

        int min_idx = 0;
        for (int i = 1; i < collision_count; i++) {
            if (potential_collisions[i].entry_time < potential_collisions[min_idx].entry_time)
                min_idx = i;
        }

        float entry_time = potential_collisions[min_idx].entry_time - 0.01f;
        if (entry_time < 0.0f) {
            entry_time = 0.0f;
        }

        Vec3 normal = potential_collisions[min_idx].normal;

        if (normal.x) {
            velocity->x = 0.0f;
            e->position.x += adjusted_velocity.x * entry_time;
        }

        if (normal.y) {
            velocity->y = 0.0f;
            e->position.y += adjusted_velocity.y * entry_time;
        }

        if (normal.z) {
            velocity->z = 0.0f;
            e->position.z += adjusted_velocity.z * entry_time;
        }

        if (normal.y > 0.0f) {
            on_ground = true;
        }
    }

    e->position.x += velocity->x * delta_time;
    e->position.y += velocity->y * delta_time;
    e->position.z += velocity->z * delta_time;
}

static float lerp(float v0, float v1, float t) {
    return (1 - t) * v0 + t * v1;
}

static float smooth_damp(float a, float b, float k, float dt) {
    return lerp(a, b, 1.0f - powf(k, dt));
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
    state.world->dirty_chunk_count = 0;
    memset(state.world, 0, sizeof(World));
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

    GLuint debug_shader =
        compile_program_from_files("res/shaders/debug.vert", "res/shaders/debug.frag");
    if (!debug_shader) {
        fprintf(stderr, "compile_program_from_files() failed\n");
        return EXIT_FAILURE;
    }

    debug_vertices = malloc(sizeof(DebugVertex) * MAX_LINES);

    GLuint debug_vao;
    GLuint debug_vbo;

    glGenVertexArrays(1, &debug_vao);
    glGenBuffers(1, &debug_vbo);

    glBindVertexArray(debug_vao);

    glBindBuffer(GL_ARRAY_BUFFER, debug_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(DebugVertex) * MAX_LINES, NULL, GL_DYNAMIC_DRAW);

    /* Position attribute */
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex),
                          (const void *)offsetof(DebugVertex, position));

    /* Color attribute */
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(DebugVertex),
                          (const void *)offsetof(DebugVertex, color));

    glBindVertexArray(0);

    state.camera.roll = 0.0f;

    state.player_aabb = (AABB){
        .position = {0, 140, 0},
        .size = {0.6f, 1.8f, 0.6f},
    };

    state.player_velocity = (Vec3){0.0f, 0.0f, 0.0f};

    float bob_time = 0.0f;
    const float BASE_CAM_Y = 1.65;
    float cam_y = BASE_CAM_Y;
    float target_cam_y = cam_y;

    float target_roll = 0.0f;

    float old_time = glfwGetTime();
    while (!glfwWindowShouldClose(state.window)) {
        glfwPollEvents();
        debug_vertex_count = 0;

        float new_time = glfwGetTime();
        float delta_time = new_time - old_time;
        old_time = new_time;

        Vec3 wish_dir = {0};
        float lmove = 0.0f;
        if (glfwGetKey(state.window, GLFW_KEY_W)) {
            wish_dir.x += state.camera.forward.x;
            wish_dir.z += state.camera.forward.z;
        }

        if (glfwGetKey(state.window, GLFW_KEY_S)) {
            wish_dir.x -= state.camera.forward.x;
            wish_dir.z -= state.camera.forward.z;
        }

        if (glfwGetKey(state.window, GLFW_KEY_D)) {
            wish_dir.x += state.camera.right.x;
            wish_dir.z += state.camera.right.z;
            lmove += 1.0f;
        }

        if (glfwGetKey(state.window, GLFW_KEY_A)) {
            wish_dir.x -= state.camera.right.x;
            wish_dir.z -= state.camera.right.z;
            lmove -= 1.0f;
        }

        if (fabs(lmove) == 0.0f) {
            target_roll = 0.0f;
        } else {
            target_roll = signf(lmove) * -to_radians(3.0f);
        }

        state.camera.roll = smooth_damp(state.camera.roll, target_roll, 0.1f, delta_time);

        wish_dir = vec3_normalize(wish_dir);

        const float MAX_SPEED = 6.0f;
        const float MAX_ACCEL = 5.0f * MAX_SPEED;
        const float GRAVITY = 9.81f * 2.2f;
        const float JUMP_HEIGHT = 1.1f;
        const float MAX_AIR_SPEED = 1.3f;
        const float FRICTION = 15.0f;

        /* We have to apply gravity even if the player is already on the ground, otherwise they will
         * oscillate up and down and `on_ground` will flicker between true and false rapidly. Swept
         * AABB should prevent tunneling so this is okay. */
        state.player_velocity.y -= GRAVITY * delta_time;

        if (on_ground) {
            float speed = vec3_len(state.player_velocity);
            if (speed > 0) {
                float drop = speed * FRICTION * delta_time;
                float new_speed = fmaxf(speed - drop, 0);
                state.player_velocity.x *= new_speed / speed;
                state.player_velocity.z *= new_speed / speed;
            }

            float wishspeed = MAX_SPEED;
            float current_speed = vec3_dot(state.player_velocity, wish_dir);
            float addspeed = wishspeed - current_speed;
            if (addspeed > 0) {
                float accelspeed = fminf(MAX_ACCEL * delta_time * wishspeed, addspeed);
                state.player_velocity =
                    vec3_add(state.player_velocity, vec3_scale(wish_dir, accelspeed));
            }

            if (glfwGetKey(state.window, GLFW_KEY_SPACE)) {
                state.player_velocity.y = sqrtf(2.0f * GRAVITY * JUMP_HEIGHT);
            }
        } else {
            float wishspeed = MAX_AIR_SPEED;
            float current_speed = vec3_dot(state.player_velocity, wish_dir);
            float addspeed = wishspeed - current_speed;
            if (addspeed > 0) {
                float accelspeed = fminf(MAX_ACCEL * delta_time * wishspeed, addspeed);
                state.player_velocity =
                    vec3_add(state.player_velocity, vec3_scale(wish_dir, accelspeed));
            }
        }

        if (vec3_len(wish_dir) > 0.0f && on_ground) {
            bob_time += delta_time;
            target_cam_y = BASE_CAM_Y + sinf(bob_time * 14.0f) * 0.23f;
        } else {
            target_cam_y = BASE_CAM_Y;
        }

        cam_y = smooth_damp(cam_y, target_cam_y, 0.01f, delta_time);

        update_collision(&state.player_aabb, &state.player_velocity, delta_time);

        state.camera.position = vec3_add(state.player_aabb.position, (Vec3){0.3f, cam_y, 0.3f});

        camera_update(&state.camera);

        iVec3 player_coord = {
            .x = state.camera.position.x / CHUNK_SIZE,
            .y = state.camera.position.y / CHUNK_SIZE,
            .z = state.camera.position.z / CHUNK_SIZE,
        };

        result = world_raycast(state.world, state.camera.position, state.camera.forward);
        if (result.did_hit) {
            Vec3 posf = vec3_from_ivec3(result.position);

            push_cube(posf, (Vec3){1.0f, 1.0f, 1.0f}, (Vec3){1.0f, 1.0f, 1.0f});
        }

        Chunk *next = pop_next_dirty(player_coord);
        if (next) {
            mesh_chunk(next->coord);
            mesh_allocator_upload(&state.allocator, &next->mesh, state.vertices,
                                  state.vertex_count);
        }

        glClearColor(0.7f, 0.7f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(state.allocator.vao);
        glUseProgram(state.shader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, textures);
        uniform_int(state.shader, "u_textures", 0);

        uniform_mat4(state.shader, "u_view", &state.camera.view);
        uniform_mat4(state.shader, "u_proj", &state.camera.proj);

        for (size_t i = 0; i < WORLD_VOLUME; i++) {
            Chunk *chunk = &state.world->chunk[i];

            if (chunk->mesh.length > 0) {
                glUniform3i(glGetUniformLocation(state.shader, "u_chunk_position"), chunk->coord.x,
                            chunk->coord.y, chunk->coord.z);

                glDrawElementsBaseVertex(GL_TRIANGLES, (chunk->mesh.length / 4) * 6,
                                         GL_UNSIGNED_INT, NULL, (int)chunk->mesh.offset);
            }
        }

        // push_cube(player_aabb.position, player_aabb.size, (Vec3){1.0, 1.0, 1.0});

        glUseProgram(0);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, debug_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(DebugVertex) * debug_vertex_count,
                        debug_vertices);

        glUseProgram(debug_shader);
        glBindVertexArray(debug_vao);
        uniform_mat4(debug_shader, "u_view", &state.camera.view);
        uniform_mat4(debug_shader, "u_proj", &state.camera.proj);

        glDrawArrays(GL_LINES, 0, debug_vertex_count);

        glUseProgram(0);

        glfwSwapBuffers(state.window);
    }

    glDeleteProgram(state.shader);

    glfwDestroyWindow(state.window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
