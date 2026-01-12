#include "meshing.h"

#include <assert.h>

#include "utils/direction.h"

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

/* Precalculated sample offsets for ambient occlusion. Each vertex of each face takes 3 samples of
 * its neighboring blocks. */
static const iVec3 AO_OFFSETS[DIRECTION_COUNT][4][3] = {
    [DIR_POSITIVE_X] =
        {
            {{ 1, 0,-1}, { 1,-1, 0}, { 1,-1,-1}},
            {{ 1, 1, 0}, { 1, 0,-1}, { 1, 1,-1}},
            {{ 1, 0, 1}, { 1, 1, 0}, { 1, 1, 1}},
            {{ 1,-1, 0}, { 1, 0, 1}, { 1,-1, 1}},
        },
    [DIR_POSITIVE_Y] =
        {
            {{ 1, 1, 0}, { 0, 1, 1}, { 1, 1, 1}},
            {{ 0, 1,-1}, { 1, 1, 0}, { 1, 1,-1}},
            {{-1, 1, 0}, { 0, 1,-1}, {-1, 1,-1}},
            {{ 0, 1, 1}, {-1, 1, 0}, {-1, 1, 1}},
        },
    [DIR_POSITIVE_Z] =
        {
            {{ 0,-1, 1}, {-1, 0, 1}, {-1,-1, 1}},
            {{ 1, 0, 1}, { 0,-1, 1}, { 1,-1, 1}},
            {{ 0, 1, 1}, { 1, 0, 1}, { 1, 1, 1}},
            {{-1, 0, 1}, { 0, 1, 1}, {-1, 1, 1}},
        },
    [DIR_NEGATIVE_X] =
        {
            {{-1, 0, 1}, {-1,-1, 0}, {-1,-1, 1}},
            {{-1, 1, 0}, {-1, 0, 1}, {-1, 1, 1}},
            {{-1, 0,-1}, {-1, 1, 0}, {-1, 1,-1}},
            {{-1,-1, 0}, {-1, 0,-1}, {-1,-1,-1}},
        },
    [DIR_NEGATIVE_Y] =
        {
            {{-1,-1, 0}, { 0,-1, 1}, {-1,-1, 1}},
            {{ 0,-1,-1}, {-1,-1, 0}, {-1,-1,-1}},
            {{ 1,-1, 0}, { 0,-1,-1}, { 1,-1,-1}},
            {{ 0,-1, 1}, { 1,-1, 0}, { 1,-1, 1}},
        },
    [DIR_NEGATIVE_Z] =
        {
            {{ 0, 1,-1}, {-1, 0,-1}, {-1, 1,-1}},
            {{ 1, 0,-1}, { 0, 1,-1}, { 1, 1,-1}},
            {{ 0,-1,-1}, { 1, 0,-1}, { 1,-1,-1}},
            {{-1, 0,-1}, { 0,-1,-1}, {-1,-1,-1}},
        },
};
/* clang-format on */

typedef struct Mesher {
    const Meshing_Data *data;
    uint32_t *vertices;
    uint32_t vertex_count;
} Mesher;

static Block_Type get_block(const Mesher *mesher, iVec3 pos) {
    int x = pos.x;
    int y = pos.y;
    int z = pos.z;

    assert(x >= -1 && x <= CHUNK_SIZE);
    assert(y >= -1 && y <= CHUNK_SIZE);
    assert(z >= -1 && z <= CHUNK_SIZE);

    size_t index = (size_t)((x + 1) + MESHING_DATA_SIZE * ((y + 1) + MESHING_DATA_SIZE * (z + 1)));

    return mesher->data->blocks[index];
}

static bool is_block_transparent(const Mesher *mesher, iVec3 pos) {
    Block_Type block = get_block(mesher, pos);
    return get_block_properties(block)->is_transparent;
}

static bool is_block_opaque(const Mesher *mesher, iVec3 pos) {
    return !is_block_transparent(mesher, pos);
}

static void push_vertex(Mesher *mesher, iVec3 pos, Direction dir, Texture_ID tex, uint8_t ao) {
    /* Vertex format:
     *  *-------------------*---------*---------------*------------*
     *  |Data               |  size   | bit range     |  max value |
     *  *-------------------*---------*---------------*------------*
     *  |x                  |  6 bits | (bits 31..26) |     64     |
     *  |y                  |  6 bits | (bits 26..20) |     64     |
     *  |z                  |  6 bits | (bits 20..14) |     64     |
     *  |direction          |  3 bits | (bits 14..11) |     8      |
     *  |ambient occlusion  |  2 bits | (bits 11..9)  |     4      |
     *  |texture id         |  9 bits | (bits  9..0)  |     512    |
     *  *-------------------*---------*---------------*------------*
     */

    assert(pos.x < 64 && pos.y < 64 && pos.z < 64);
    assert(dir < 8);
    assert(ao < 4);
    assert(tex < 512);

    /* clang-format off */
    uint32_t vert = ((uint32_t)pos.x << 26)   |
                    ((uint32_t)pos.y << 20)   |
                    ((uint32_t)pos.z << 14)   |
                    ((uint32_t)dir << 11) |
                    ((uint32_t)ao << 9)   |
                    (uint32_t)tex;
    /* clang-format on */

    assert(mesher->vertex_count < MAX_VERTS);
    mesher->vertices[mesher->vertex_count] = vert;
    mesher->vertex_count++;
}

static uint8_t vertex_ao(bool side_1, bool side_2, bool corner) {
    if (side_1 && side_2) {
        return 0;
    }

    return 3 - (side_1 + side_2 + corner);
}

static void push_face(Mesher *mesher, iVec3 pos, Direction dir, Texture_ID tex) {
    for (int i = 0; i < 4; i++) {
        iVec3 side_1_sample = AO_OFFSETS[dir][i][0];
        iVec3 side_2_sample = AO_OFFSETS[dir][i][1];
        iVec3 corner_sample = AO_OFFSETS[dir][i][2];

        bool side_1 = is_block_opaque(mesher, ivec3_add(pos, side_1_sample));
        bool side_2 = is_block_opaque(mesher, ivec3_add(pos, side_2_sample));
        bool corner = is_block_opaque(mesher, ivec3_add(pos, corner_sample));
        uint8_t ao = vertex_ao(side_1, side_2, corner);

        iVec3 vertex_pos = ivec3_add(pos, FACE_VERTICES[dir][i]);
        push_vertex(mesher, vertex_pos, dir, tex, ao);
    }
}

static void mesh_block(Mesher *mesher, Block_Type type, iVec3 pos) {
    for (Direction dir = 0; dir < DIRECTION_COUNT; dir++) {
        iVec3 neighbor_pos = ivec3_add(pos, direction_to_ivec3(dir));

        /* The face is exposed if its neighbor is transparent. */
        if (is_block_transparent(mesher, neighbor_pos)) {
            const Block_Properties *properties = get_block_properties(type);
            push_face(mesher, pos, dir, properties->textures[dir]);
        }
    }
}

uint32_t *mesh_chunk(const Meshing_Data *data, uint32_t *vertex_count, Arena *arena) {
    assert(data != NULL);
    assert(vertex_count != NULL);
    assert(arena != NULL);

    Mesher mesher = {
        .data = data,
        .vertex_count = 0,
        .vertices = ARENA_NEW_ARRAY(arena, uint32_t, MAX_VERTS),
    };

    for (int z = 0; z < CHUNK_SIZE; z++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int x = 0; x < CHUNK_SIZE; x++) {
                iVec3 pos = {x, y, z};
                Block_Type type = get_block(&mesher, pos);

                if (type != BLOCK_AIR) {
                    mesh_block(&mesher, type, pos);
                }
            }
        }
    }

    *vertex_count = mesher.vertex_count;
    return mesher.vertices;
}

uint32_t *generate_index_buffer(uint32_t *index_count, Arena *arena) {
    assert(index_count != NULL);
    assert(arena != NULL);

    uint32_t *buffer = ARENA_NEW_ARRAY(arena, uint32_t, MAX_INDICES);
    const uint32_t INDEX_PATTERN[] = {0, 1, 3, 1, 2, 3};

    for (uint32_t i = 0; i < MAX_INDICES; i++) {
        buffer[i] = (i / 6) * 4 + INDEX_PATTERN[i % 6];
    }

    *index_count = MAX_INDICES;
    return buffer;
}
