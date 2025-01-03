#ifndef SHADERS_H
#define SHADERS_H

#include <GLES2/gl2.h>  // for GLuint, GLenum

GLuint load_shader(GLenum type, const char *source);
GLuint create_program(const char *vertex_source, const char *fragment_source);

#endif // SHADERS_H
