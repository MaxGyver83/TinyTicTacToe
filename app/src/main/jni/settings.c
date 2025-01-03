#include "settings.h"
#include <stdbool.h>   // for bool, false, true
#include <stdint.h>    // for int32_t
#include "graphics.h"  // for Rectangle, render_texture, Pixel, Texture, ALI...

#define LEVEL_COUNT 5

extern int32_t win_width;
extern int32_t win_height;
extern int difficulty;
extern Texture t_difficulty;
extern Texture t_levels[];
extern Texture t_digits[10];

bool show_settings = false;
Rectangle settings_window_rect = {0.0f, 0.0f, 1.0f, 1.0f};
Rectangle b_difficulty[LEVEL_COUNT];

static const Pixel bgcolor = {101, 252, 204, 255};
static const Pixel bordercolor = {0, 0, 0, 255};

static bool initialized = false;
static float center_x;
static float center_y;
static float settings_window_height;
static float text_height_title;
static float line_height_title;
static float text_height;
static float line_height;


static void
initialize(void)
{
	float padding = win_width * 0.05f;
	text_height_title = win_width * 0.08f;
	line_height_title = text_height_title * 1.2f;
	settings_window_height = padding * 2 + line_height_title;
	text_height = win_width * 0.06f;
	line_height = text_height * 1.2f;
	settings_window_height += text_height + (LEVEL_COUNT - 1) * line_height;
	settings_window_rect = (Rectangle){
		.x = win_width * 0.25f,
			.y = (win_height - settings_window_height) / 2,
			.w = win_width * 0.5f,
			.h = settings_window_height,
	};
	center_x = win_width / 2.0f;
	center_y = win_height / 2.0f;
	initialized = true;
}

void
render_window()
{
	if (!initialized)
		initialize();

	Rectangle r = settings_window_rect;
	draw_filled_rectangle(bgcolor, r);
	draw_rectangle(bordercolor, 5.0f, r.x, r.y, r.w, r.h);

	float padding = win_width * 0.05f;
	float y = r.y + padding;
	render_texture(t_difficulty, center_x, y, 0.0f, text_height_title, ALIGN_TOP_CENTER);
	y += line_height_title;
	float x = r.x + padding;
	Rectangle level;
	for (int i = 0; i < LEVEL_COUNT; i++) {
		b_difficulty[i] = render_texture(t_digits[i+1], x, y, 0, text_height, ALIGN_TOP_LEFT);
		level = render_texture(t_levels[i], x + b_difficulty[i].w + padding, y,
				0, text_height, ALIGN_TOP_LEFT);
		y += line_height;
	}
	for (int i = 0; i < LEVEL_COUNT; i++)
		b_difficulty[i].w += padding + level.w; // width of "very hard" label

	Rectangle b = b_difficulty[difficulty-1];
	draw_rectangle_centered(
		(Pixel){0, 0, 255, 255},
		5.0f,
		b.x + b.w / 2,
		b.y + b.h / 2,
		b.w + padding,
		line_height
	);
}
