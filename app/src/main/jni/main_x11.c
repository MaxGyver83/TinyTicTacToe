#include <EGL/eglplatform.h>  // for EGLNativeDisplayType
#include <X11/X.h>            // for Button1, ButtonPressMask, ButtonRelease...
#include <X11/XKBlib.h>       // for XkbKeycodeToKeysym
#include <X11/Xatom.h>        // for XA_WM_NORMAL_HINTS
#include <X11/Xlib.h>         // for XCheckMaskEvent, XCloseDisplay, XCreate...
#include <X11/Xutil.h>        // for XSetSizeHints, PMaxSize, PMinSize, XSiz...
#include <X11/keysym.h>       // for XK_p, XK_space, XK_Escape, XK_q
#include <err.h>              // for errx
#include <inttypes.h>         // for int64_t
#include <stdbool.h>          // for true, bool, false
#include <stdio.h>            // for printf, NULL
#include <stdlib.h>           // for atoi
#include <string.h>           // for strlen
#include <unistd.h>           // for usleep
#include "gamelogic.h"        // for LEVEL_COUNT
#include "init.h"             // for main_loop_step, init, shutdown_all, g_i...
#include "mouse.h"            // for mouse, mouse_init, mouse_reset
#include "utils.h"            // for get_timestamp
#include "utils_x11.h"        // for streq

#define TARGET_FPS 20
#define TARGET_FRAME_TIME (1.0f / TARGET_FPS)

#define POSX 20
#define POSY 15
#define HEIGHT 1000
// Aspect ratios for testing:
//  4:3  = 1.333 (Samsung Galaxy Y)
// 16:9  = 1.777 (OnePlus 3)
// 37:18 = 2.055 (Samsung Galaxy S8)
// 20:9  = 2.222 (OnePlus Nord)
#define ASPECT_RATIO 1.777
#define WIDTH (HEIGHT / ASPECT_RATIO)

extern char key_pressed;

Display *dpy;
int scr;
Visual *vis;
Window root, win;
int start_level = 0; // 0 = read from stats.txt
char *prefill = NULL;

static int height = HEIGHT;
static int width = WIDTH;

static void
usage(int exit_code)
{
	printf("usage: tinytictactoe [-h] [--full-hd] [--qHD] "
	       "[--height WIN_HEIGHT] [--size WIDTHxHEIGHT] "
	       "[-l LEVEL] [--prefill XO-...]\n");
	exit(exit_code);
}

static void
set_wm_class(const char *res_name, const char *res_class)
{
	XClassHint *classHint = XAllocClassHint();
	classHint->res_name = (char *)res_name;  // Usually argv[0]
	classHint->res_class = (char *)res_class; // Application name
	XSetClassHint(dpy, win, classHint);
	XFree(classHint);
}

static void
set_net_wm_name(const char *title)
{
	Atom netWmName = XInternAtom(dpy, "_NET_WM_NAME", False);
	Atom utf8String = XInternAtom(dpy, "UTF8_STRING", False);
	XChangeProperty(dpy, win, netWmName, utf8String, 8, PropModeReplace,
			(unsigned char *)title, strlen(title));
	XFlush(dpy);
}

static bool
process_input(void)
{
	XEvent ev;
	long event_mask = KeyPressMask | KeyReleaseMask
			| ButtonPressMask | ButtonReleaseMask | StructureNotifyMask;
	while (XCheckMaskEvent(dpy, event_mask, &ev)) {
		KeySym key;
		switch (ev.type) {
			case ButtonPress:
				if (ev.xbutton.button == Button1) {
					mouse.x = ev.xbutton.x;
					mouse.y = ev.xbutton.y;
					mouse.is_down = true;
				}
				break;

			case ButtonRelease:
				if (ev.xbutton.button == Button1) {
					mouse.x = ev.xbutton.x;
					mouse.y = ev.xbutton.y;
					mouse.is_released = true;
				}
				break;

			case KeyPress:
				key = XkbKeycodeToKeysym(dpy, ev.xkey.keycode, 0, 0);
				if ((key >= XK_0 && key <= XK_9) || key == XK_s) {
					key_pressed = key;
				} else if (key == XK_Escape) {
					key_pressed = '\e';
				} else if (key == XK_space || key == XK_n) {
					// Simulate click (in the corner of the window)
					// to start a new game
					mouse.x = 0.01f * width;
					mouse.y = 0.01f * height;
					mouse.is_down = true;
				} else if (key == XK_q) {
					// quit
					return false;
				}
				break;

			case KeyRelease:
				key = XkbKeycodeToKeysym(dpy, ev.xkey.keycode, 0, 0);
				if (key == XK_space || key == XK_p) {
					mouse.x = 0.27f * width;
					mouse.y = 0.68f * height;
					mouse.is_released = true;
				}
				break;

			case ConfigureNotify:
				main_loop_step();
				break;
		}
	}
	return true;
}

static void
game_loop(void)
{
	while (true) {
		int64_t ts_start = get_timestamp();
		if (!process_input())
			break;
		main_loop_step();
		mouse_reset(&mouse);
		int64_t microsecs_passed = get_timestamp() - ts_start;
		int64_t sleep_time = TARGET_FRAME_TIME * 1000000 - microsecs_passed;
		if (sleep_time > 0)
			usleep(sleep_time);
	}
}

int
main(int argc, char *argv[])
{
	for (int i = 1; i < argc; i++) {
		if (streq(argv[i], "-h")) {
			usage(0);
		} else if (streq(argv[i], "--full-hd")) {
			height = 1920;
			width = 1080;
		} else if (streq(argv[i], "--qHD")) {
			height = 960;
			width = 540;
		} else if (streq(argv[i], "-l")) {
			i++;
			if (i >= argc)
				usage(1);
			start_level = atoi(argv[i]);
			debug("LEVEL = %d", start_level);
			if (start_level < 1 || start_level > LEVEL_COUNT) {
				error("LEVEL must be between 1 and %i!", LEVEL_COUNT);
				exit(1);
			}
		} else if (streq(argv[i], "--height")) {
			i++;
			if (i >= argc)
				usage(1);
			height = atoi(argv[i]);
			width = height / ASPECT_RATIO;
		} else if (streq(argv[i], "--size")) {
			i++;
			if (i >= argc)
				usage(1);
			char *p = strtok(argv[i], "x,");
			if (!p)
				usage(1);
			width = atoi(p);
			p = strtok(NULL, "x,");
			if (!p)
				usage(1);
			height = atoi(p);
			if (width == 0 || height == 0) {
				error("width and height must not be 0.");
				exit(1);
			}
		} else if (streq(argv[i], "--prefill")) {
			i++;
			if (i >= argc)
				usage(1);
			prefill = argv[i];
			if (strlen(prefill) != 9) {
				error("Invalid prefill string: %s\nString length must be 9. Example: X--O--X-O", prefill);
				usage(1);
			}
		} else {
			error("Unknown argument: %s", argv[i]);
			usage(1);
		}
	}

	mouse_init(&mouse);

	if (!(dpy = XOpenDisplay(NULL)))
		errx(1, "Can't open display");

	scr = DefaultScreen(dpy);
	root = RootWindow(dpy, scr);
	vis = DefaultVisual(dpy, scr);
	XSetWindowAttributes xwa = {
		.event_mask =
			Button1MotionMask | ButtonPressMask | ButtonReleaseMask
			| KeyPressMask| KeyReleaseMask | StructureNotifyMask,
	};
	win = XCreateWindow(dpy, root, POSX, POSY, width, height, 3,
			DefaultDepth(dpy, scr), InputOutput, vis,
			CWBackPixel | CWEventMask | CWBorderPixel, &xwa);

	XSizeHints xsh = {
		.flags = PMinSize | PMaxSize,
	};
	XSetSizeHints(dpy, win, &xsh, XA_WM_NORMAL_HINTS);

	char *title = "Tiny Tic Tac Toe";
	XStoreName(dpy, win, title);
	set_wm_class("tinytictactoe", title);
	set_net_wm_name(title);
	XMapWindow(dpy, win);
	XMapSubwindows(dpy, win);

	init((EGLNativeDisplayType)dpy, &win);
	if (!g_initialized)
		return 1;

	game_loop();

	/* Clean up */
	shutdown_all();
	XUnmapWindow(dpy, win);
	XUnmapSubwindows(dpy, win);
	XDestroySubwindows(dpy, win);
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);

	return 0;
}
