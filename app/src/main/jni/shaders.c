#include "shaders.h"
#include <GLES2/gl2.h>  // for GLuint, glAttachShader, GL_FALSE, GLchar, GLint
#include <stddef.h>     // for NULL
#include "utils.h"      // for info

GLuint
load_shader(GLenum type, const char *source)
{
	// check if OpenGL context is initialized:
	// printf("OpenGL version is %s\n", glGetString(GL_VERSION));
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		GLchar info_log[512];
		glGetShaderInfoLog(shader, 512, NULL, info_log);
		info("Shader compilation failed: %s", info_log);
	}
	return shader;
}

GLuint
create_program(const char *vertex_source, const char *fragment_source)
{
	GLuint vertex_shader = load_shader(GL_VERTEX_SHADER, vertex_source);
	GLuint fragment_shader = load_shader(GL_FRAGMENT_SHADER, fragment_source);

	GLuint program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);

	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		GLchar info_log[512];
		glGetProgramInfoLog(program, 512, NULL, info_log);
		info("ERROR::PROGRAM::LINKING_FAILED: %s", info_log);
	}
	return program;
}
