#ifndef UTILS_H
#define UTILS_H

#include <glad/gl.h>

char *slurp_file_str(const char *filename);
GLuint compile_shader(const char *source, GLenum type);
GLuint compile_program(GLuint vert, GLuint frag);
GLuint compile_program_from_files(const char *vert_filename, const char *frag_filename);

#endif /* UTILS_H */
