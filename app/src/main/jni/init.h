#ifndef INIT_H
#define INIT_H

#include <EGL/eglplatform.h>  // for EGLNativeDisplayType
#include <GLES2/gl2.h>        // for GLuint
#include <stdbool.h>          // for bool
#include <stdint.h>           // for int32_t

extern bool g_initialized;
extern struct android_app *g_app;
extern int32_t win_width;
extern int32_t win_height;

extern GLuint texture_program;
extern GLuint color_program;
extern GLuint g_position_handle;
extern GLuint g_color_handle;

bool update_window_size(void);
void init(EGLNativeDisplayType dpy, void *native_window);
void init_android(struct android_app *app);
void main_loop_step(void);
void redraw(void);
void shutdown_all(void);

#endif // INIT_H
