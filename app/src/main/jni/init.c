#include "init.h"
#include <EGL/egl.h>                  // for EGL_NO_CONTEXT, EGL_NO_SURFACE
#include <GLES2/gl2.h>                // for GLchar, GLuint, glDeleteProgram
#ifndef X11
#include <android/native_window.h>    // for ANativeWindow_acquire, ANativeW...
#include <android_native_app_glue.h>  // for android_app
#endif
#include <stdio.h>                    // for NULL
#include "audio.h"                    // for create_audio_engine
#include "gameflow.h"                 // for init_game, render, shutdown_game
#include "shaders.h"                  // for create_program
#include "utils.h"                    // for info, error

#ifdef X11
#define ANativeWindow_acquire(x)
#define ANativeWindow_setBuffersGeometry(...)
#define ANativeWindow_release(x)
#endif

bool g_initialized = false;
struct android_app *g_app = NULL;
char stats_filepath[256] = "./stats.txt";

int32_t win_width = 0;
int32_t win_height = 0;
float max_line_width = 1.0f;

GLuint texture_program;
GLuint color_program;
GLuint g_position_handle;
GLuint g_color_handle;

static EGLDisplay egl_display = EGL_NO_DISPLAY;
static EGLSurface egl_surface = EGL_NO_SURFACE;
static EGLContext egl_context = EGL_NO_CONTEXT;

static const GLchar *vertex_shader_texture =
"attribute vec4 a_position;"
"attribute vec2 a_tex_coord;"
"varying vec2 v_tex_coord;"
"void main() {"
"    gl_Position = a_position;"
"    v_tex_coord = a_tex_coord;"
"}";

static const GLchar *fragment_shader_texture =
"precision mediump float;"
"varying vec2 v_tex_coord;"
"uniform sampler2D u_texture;"
"void main() {"
"    gl_FragColor = texture2D(u_texture, v_tex_coord);"
"}";

// Vertex and fragment shader source code
static const GLchar *vertex_shader_color = //"#version 120\n"
"attribute vec2 a_position;"
"void main() {"
"    gl_Position = vec4(a_position, 0.0, 1.0);"
"}";

static const GLchar *fragment_shader_color = //"#version 120\n"
"precision mediump float;"
"uniform vec4 u_color;"
"void main() {"
"    gl_FragColor = u_color;"
"}";

bool
update_window_size()
{
	int32_t new_win_width, new_win_height;
	eglQuerySurface(egl_display, egl_surface, EGL_WIDTH, &new_win_width);
	eglQuerySurface(egl_display, egl_surface, EGL_HEIGHT, &new_win_height);

	if (win_width == new_win_width && win_height == new_win_height)
		return false;

	info("Window resized: %d x %d -> %d x %d",
			win_width, win_height, new_win_width, new_win_height);
	win_width = new_win_width;
	win_height = new_win_height;
	update_geometry();
	return true;
}

void
init(EGLNativeDisplayType dpy, void *native_window)
{
	if (g_initialized)
		return;
	debug("Initialize.");

#ifndef X11
	ANativeWindow_acquire(g_app->window);
	snprintf(stats_filepath, sizeof(stats_filepath), "%s/stats.txt", g_app->activity->internalDataPath);
#endif

	// Initialize EGL
	egl_display = eglGetDisplay(dpy ? dpy : EGL_DEFAULT_DISPLAY);
	if (egl_display == EGL_NO_DISPLAY) {
		error("eglGetDisplay(EGL_DEFAULT_DISPLAY) returned EGL_NO_DISPLAY");
		return;
	}

	if (eglInitialize(egl_display, 0, 0) != EGL_TRUE) {
		error("eglInitialize() returned with an error");
		return;
	}

	const EGLint egl_attributes[] = {
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_NONE
	};
	EGLint num_configs = 0;
	if (eglChooseConfig(egl_display, egl_attributes, NULL, 0, &num_configs) != EGL_TRUE) {
		error("eglChooseConfig() returned with an error");
		return;
	}
	if (num_configs == 0) {
		error("eglChooseConfig() returned 0 matching config");
		return;
	}

	EGLConfig egl_config;
	eglChooseConfig(egl_display, egl_attributes, &egl_config, 1, &num_configs);
	EGLint egl_format;
	eglGetConfigAttrib(egl_display, egl_config, EGL_NATIVE_VISUAL_ID, &egl_format);
	ANativeWindow_setBuffersGeometry(g_app->window, 0, 0, egl_format);

	const EGLint egl_context_attributes[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
	egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, egl_context_attributes);
	if (egl_context == EGL_NO_CONTEXT) {
		error("eglCreateContext() returned EGL_NO_CONTEXT");
		return;
	}

#ifdef X11
	egl_surface = eglCreatePlatformWindowSurface(egl_display, egl_config, native_window, NULL);
#else
	egl_surface = eglCreateWindowSurface(egl_display, egl_config, (EGLNativeWindowType)(g_app->window), NULL);
#endif
	if (egl_surface == EGL_NO_SURFACE) {
		error("eglCreateWindowSurface() returned EGL_NO_SURFACE");
		return;
	}

	if (eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context) != EGL_TRUE) {
		error("eglMakeCurrent() returned with an error");
		return;
	}

	GLfloat range[2];
	glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, range);
	// My laptop: line width range is from 1.0 to 7.375
	// One Plus Nord 2: line width range is from 1.0 to 4095.9375
	max_line_width = range[1];

	// Set window size
	update_window_size();

	if (!init_game()) {
		error("Game not initialized!");
		return;
	}

	create_audio_engine();

	// Create shader program
	texture_program = create_program(vertex_shader_texture, fragment_shader_texture);
	color_program = create_program(vertex_shader_color, fragment_shader_color);
	g_position_handle = glGetAttribLocation(color_program, "a_position");
	g_color_handle = glGetUniformLocation(color_program, "u_color");

	debug("Tiny Tic Tac Toe is initialized!");

	g_initialized = true;
}

void
init_android(struct android_app *app)
{
	g_app = app;
	init(EGL_DEFAULT_DISPLAY, NULL);
}

void
main_loop_step()
{
	if (egl_display == EGL_NO_DISPLAY)
		return;

	bool updated = update();
	bool resized = update_window_size();
	if (updated || resized) {
		render();
		eglSwapBuffers(egl_display, egl_surface);
	}
}

void
shutdown_all()
{
	info("Shutdown");
	if (!g_initialized)
		return;

	// Cleanup
	if (egl_display != EGL_NO_DISPLAY) {
		eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

		if (egl_context != EGL_NO_CONTEXT)
			eglDestroyContext(egl_display, egl_context);

		if (egl_surface != EGL_NO_SURFACE)
			eglDestroySurface(egl_display, egl_surface);

		eglTerminate(egl_display);
	}

	egl_display = EGL_NO_DISPLAY;
	egl_context = EGL_NO_CONTEXT;
	egl_surface = EGL_NO_SURFACE;
	ANativeWindow_release(g_app->window);

	shutdown_game();
	glDeleteProgram(texture_program);
	glDeleteProgram(color_program);

	g_initialized = false;
}
