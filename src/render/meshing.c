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

static uint32_t pack_vertex(uint8_t x, uint8_t y, uint8_t z, uint8_t dir, uint8_t ao,
                            uint16_t tex) {
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

    assert(x < 64 && y < 64 && z < 64);
    assert(dir < 8);
    assert(ao < 4);
    assert(tex < 512);

    /* clang-format off */
    return ((uint32_t)x << 26)   |
           ((uint32_t)y << 20)   |
           ((uint32_t)z << 14)   |
           ((uint32_t)dir << 11) |
           ((uint32_t)ao << 9)   |
            (uint32_t)tex;
    /* clang-format on */
}

typedef struct Mesher {
    uint32_t *vertices;
    uint32_t vertex_count;
} Mesher;

static void push_vertex(Mesher *mesher, iVec3 pos, Direction direction, Texture_ID texture,
                        uint8_t ao) {
    assert(mesher->vertex_count < MAX_VERTS);

    uint32_t vert = pack_vertex(pos.x, pos.y, pos.z, direction, ao, texture);

    mesher->vertices[mesher->vertex_count] = vert;
    mesher->vertex_count++;
}

static uint8_t vertex_ao(bool side_1, bool side_2, bool corner) {
    if (side_1 && side_2) {
        return 0;
    }

    return 3 - (side_1 + side_2 + corner);
}

static void emit_face(Mesher *mesher, const Chunk *chunk, iVec3 pos, Direction direction,
                      Texture_ID texture) {
    for (int i = 0; i < 4; i++) {
        iVec3 side_1_pos = AO_OFFSETS[direction][i][0];
        iVec3 side_2_pos = AO_OFFSETS[direction][i][1];
        iVec3 corner_pos = AO_OFFSETS[direction][i][2];

        bool corner_opaque = !chunk_is_block_transparent(chunk, ivec3_add(pos, corner_pos));
        bool side_1_opaque = !chunk_is_block_transparent(chunk, ivec3_add(pos, side_1_pos));
        bool side_2_opaque = !chunk_is_block_transparent(chunk, ivec3_add(pos, side_2_pos));

        uint8_t ao = vertex_ao(side_1_opaque, side_2_opaque, corner_opaque);
        push_vertex(mesher, ivec3_add(pos, FACE_VERTICES[direction][i]), direction, texture, ao);
    }
}

static void mesh_block(Mesher *mesher, const Chunk *chunk, Block_Type type, iVec3 pos) {
    for (Direction dir = 0; dir < DIRECTION_COUNT; dir++) {
        iVec3 neighbor_pos = ivec3_add(pos, direction_to_ivec3(dir));
        if (chunk_is_block_transparent(chunk, neighbor_pos)) {
            const Block_Properties *properties = get_block_properties(type);
            emit_face(mesher, chunk, pos, dir, properties->textures[dir]);
        }
    }
}

uint32_t *mesh_chunk(const Chunk *chunk, uint32_t *vertex_count, Arena *arena) {
    assert(chunk != NULL);
    assert(vertex_count != NULL);
    assert(arena != NULL);

    Mesher mesher = {
        .vertex_count = 0,
        .vertices = ARENA_NEW_ARRAY(arena, uint32_t, MAX_VERTS),
    };

    for (int z = 0; z < CHUNK_SIZE; z++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int x = 0; x < CHUNK_SIZE; x++) {
                iVec3 pos = {x, y, z};

                Block_Type type = chunk_get_block_unsafe(chunk, pos);
                if (type != BLOCK_AIR) {
                    mesh_block(&mesher, chunk, type, pos);
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
