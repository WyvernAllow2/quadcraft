#ifndef UTILS_H
#define UTILS_H

#include <glad/gl.h>
#include "math3d.h"

char *slurp_file_str(const char *filename);
GLuint compile_shader(const char *source, GLenum type);
GLuint compile_program(GLuint vert, GLuint frag);
GLuint compile_program_from_files(const char *vert_filename, const char *frag_filename);
void uniform_mat4(GLuint program, const char *name, const Mat4 *value);
void uniform_int(GLuint program, const char *name, int value);
void uniform_vec3(GLuint program, const char *name, Vec3 value);

#endif /* UTILS_H */
