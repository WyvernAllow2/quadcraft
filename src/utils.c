#include "utils.h"

#include <assert.h>
#include <glad/gl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "arena.h"

char *read_file(const char *filename, Arena *arena) {
    assert(filename != NULL);
    assert(arena != NULL);

    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror(filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = (size_t)ftell(file);
    fseek(file, 0, SEEK_SET);

    /* +1 for null-terminator */
    char *buffer = ARENA_NEW_ARRAY(arena, char, file_size + 1);
    if (!buffer) {
        perror(filename);
        fclose(file);
        return NULL;
    }

    if (fread(buffer, 1, file_size, file) != file_size) {
        perror(filename);
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[file_size] = '\0';
    fclose(file);
    return buffer;
}

GLuint compile_shader(const char *source, GLenum type) {
    assert(source);

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

GLuint compile_program(GLuint vert, GLuint frag) {
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

GLuint compile_program_from_files(const char *vert_filename, const char *frag_filename,
                                  Arena *arena) {
    const char *vert_source = read_file(vert_filename, arena);
    if (!vert_source) {
        fprintf(stderr, "Failed to load vertex shader source: %s\n", vert_filename);
        return 0;
    }

    const char *frag_source = read_file(frag_filename, arena);
    if (!frag_source) {
        fprintf(stderr, "Failed to load fragment shader source: %s\n", frag_filename);
        return 0;
    }

    GLuint vert = compile_shader(vert_source, GL_VERTEX_SHADER);
    GLuint frag = compile_shader(frag_source, GL_FRAGMENT_SHADER);
    if (!vert || !frag) {
        return 0;
    }

    return compile_program(vert, frag);
}
