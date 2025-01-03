#include <android/input.h>            // for AInputEvent_getType, AKeyEvent_...
#include <android/keycodes.h>         // for AKEYCODE_A, AKEYCODE_Q, AKEYCODE_Z
#include <android/looper.h>           // for ALooper_pollOnce, ALOOPER_POLL_...
#include <stdbool.h>                  // for false, true, bool
#include <stddef.h>                   // for NULL, size_t
#include <stdint.h>                   // for int32_t, int64_t
#include <stdlib.h>                   // for exit
#include <unistd.h>                   // for usleep
#include <android_native_app_glue.h>  // for android_app, APP_CMD_GAINED_FOCUS
#include "init.h"                     // for shutdown, init, main_loop_step
#include "mouse.h"                    // for mouse, mouse_init
#include "utils.h"                    // for get_timestamp, debug

#define TARGET_FPS 20
#define TARGET_FRAME_TIME (1.0f / TARGET_FPS)


static const char *
get_app_cmd_string(int32_t cmd)
{
	const char *APP_CMDS[] = {
		"APP_CMD_INPUT_CHANGED",
		"APP_CMD_INIT_WINDOW",
		"APP_CMD_TERM_WINDOW",
		"APP_CMD_WINDOW_RESIZED",
		"APP_CMD_WINDOW_REDRAW_NEEDED",
		"APP_CMD_CONTENT_RECT_CHANGED",
		"APP_CMD_GAINED_FOCUS",
		"APP_CMD_LOST_FOCUS",
		"APP_CMD_CONFIG_CHANGED",
		"APP_CMD_LOW_MEMORY",
		"APP_CMD_START",
		"APP_CMD_RESUME",
		"APP_CMD_SAVE_STATE",
		"APP_CMD_PAUSE",
		"APP_CMD_STOP",
		"APP_CMD_DESTROY",
	};
	return APP_CMDS[cmd];
}

static void
handle_app_cmd(struct android_app *app, int32_t cmd)
{
	debug("handle_app_cmd: %s\n", get_app_cmd_string(cmd));
	switch (cmd) {
		case APP_CMD_INIT_WINDOW:
			init_android(app);
			break;
		/* case APP_CMD_CONFIG_CHANGED: */
		/* case APP_CMD_WINDOW_RESIZED: */
		/* case APP_CMD_CONTENT_RECT_CHANGED: */
		case APP_CMD_WINDOW_REDRAW_NEEDED:
			if (g_initialized)
				main_loop_step();
			break;
		case APP_CMD_TERM_WINDOW:
			shutdown_all();
			exit(0);
			break;
	}
}

int32_t
handle_input(struct android_app *app, AInputEvent *event)
{
	int32_t eventType = AInputEvent_getType(event);

	if (eventType == AINPUT_EVENT_TYPE_MOTION) {
		int32_t action = AMotionEvent_getAction(event);
		action &= AMOTION_EVENT_ACTION_MASK;
		int whichsource = action >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
		size_t pointer_count = AMotionEvent_getPointerCount(event);
		float x = 0.0f;
		float y = 0.0f;
		bool is_down = false;
		bool is_released = false;
		bool is_moved = false;
		int index;

		for (size_t i = 0; i < pointer_count; ++i) {
			x = AMotionEvent_getX(event, i);
			y = AMotionEvent_getY(event, i);
			index = AMotionEvent_getPointerId(event, i);

			if (action == AMOTION_EVENT_ACTION_POINTER_DOWN || action == AMOTION_EVENT_ACTION_DOWN) {
				int id = index;
				if (action == AMOTION_EVENT_ACTION_POINTER_DOWN && id != whichsource) continue;

				mouse.x = mouse.x0 = x;
				mouse.y = mouse.y0 = y;
				mouse.is_down = true;
				mouse.is_released = false;
			} else if (action == AMOTION_EVENT_ACTION_POINTER_UP || action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
				int id = index;
				if (action == AMOTION_EVENT_ACTION_POINTER_UP && id != whichsource) continue;

				mouse.x = x;
				mouse.y = y;
				mouse.is_released = true;
				mouse.is_moved = false;
				mouse.is_down = false;
				mouse.x_traveldistance += mouse.x - mouse.x0;
				mouse.y_traveldistance += mouse.y - mouse.y0;
				mouse.x0 = mouse.y0 = 0.0f;
			} else if (action == AMOTION_EVENT_ACTION_MOVE) {
				mouse.is_moved = true;

				mouse.x = x;
				mouse.y = y;
			}
		}

		return 1;
	} else if (eventType == AINPUT_EVENT_TYPE_KEY) { // Apparently, this applies to physical keys, including volume control keys.
		if (AKeyEvent_getAction(event) == AKEY_EVENT_ACTION_UP) {
			int32_t key_val = AKeyEvent_getKeyCode(event);
			if (key_val == AKEYCODE_Q)
				app->destroyRequested = 1;
		}
		return 0;
	}
	return 0; //Return 0 if you are not processing the event
}

void
android_main(struct android_app* state)
{
	state->onInputEvent = handle_input;
	state->onAppCmd = handle_app_cmd;

	mouse_init(&mouse);

	while (true) {
		int ident;
		int events;
		struct android_poll_source *source;
		int64_t ts_start = get_timestamp();

		// Poll all events until we reach return value TIMEOUT, meaning no events left to process
		while ((ident = ALooper_pollOnce(0, NULL, &events, (void**)&source)) > ALOOPER_POLL_TIMEOUT) {
			if (source)
				source->process(state, source);

			if (state->destroyRequested) {
				shutdown_all();
				exit(0);
				return;
			}
		}

		if (g_initialized) {
			main_loop_step();
			/* mouse_reset(&mouse); */
			int64_t microsecs_passed = get_timestamp() - ts_start;
			int64_t sleep_time = TARGET_FRAME_TIME * 1000000 - microsecs_passed;
			if (sleep_time > 0)
				usleep(sleep_time);
		}
	}
}
