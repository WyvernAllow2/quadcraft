#include "texture_array.h"

#include <stb_image.h>
#include <stdbool.h>
#include <stdio.h>

#include "texture_id.h"

GLuint load_texture_array(void) {
    stbi_set_flip_vertically_on_load(true);

    GLuint texture_array;
    glGenTextures(1, &texture_array);
    glBindTexture(GL_TEXTURE_2D_ARRAY, texture_array);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_SRGB8_ALPHA8, TEXTURE_SIZE, TEXTURE_SIZE,
                   TEXTURE_ID_COUNT);

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

        if (width != TEXTURE_SIZE || height != TEXTURE_SIZE) {
            fprintf(stderr, "Invalid texture size: %s\n", get_texture_filename(i));
            continue;
        }

        glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, width, height, 1, GL_RGBA,
                        GL_UNSIGNED_BYTE, data);

        stbi_image_free(data);
    }

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    return texture_array;
}
