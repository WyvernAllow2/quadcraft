#ifndef UTILS_H
#define UTILS_H

#include <assert.h>
#include <glad/gl.h>
#include <stddef.h>

#include "arena.h"

char *read_file(const char *filename, Arena *arena);
GLuint compile_shader(const char *source, GLenum type);
GLuint compile_program(GLuint vert, GLuint frag);
GLuint compile_program_from_files(const char *vert_filename, const char *frag_filename,
                                  Arena *arena);

#endif /* UTILS_H */
