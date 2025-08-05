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
extern const Color bgcolor;
extern float padding_button_h;
extern float padding_button_v;

bool show_settings = false;
Rectangle settings_window_rect = {0.0f, 0.0f, 1.0f, 1.0f};
Rectangle b_difficulty[LEVEL_COUNT];

static const Color shadowcolor = {0, 0, 0, 0.5f};
static const Color bordercolor = {0, 0, 0, 1.0f};

static bool initialized = false;
static float center_x;
static float center_y;
static float settings_window_height;
static float text_height_title;
static float line_height_title;
static float text_height;
static float button_height;
static float button_width;
static float line_height;


static void
initialize(void)
{
	float padding = win_width * 0.05f;
	text_height_title = win_width * 0.12f;
	line_height_title = text_height_title * 1.2f;
	settings_window_height = padding * 2 + line_height_title;
	text_height = win_width * 0.10f;
	button_height = text_height * 1.2f;
	line_height = button_height * 1.3f;
	button_width = text_height * t_digits[LEVEL_COUNT].w / t_digits[LEVEL_COUNT].h
		+ padding / 2 + text_height * t_levels[LEVEL_COUNT-1].w / t_levels[LEVEL_COUNT-1].h
		+ 2 * padding_button_h;
	float settings_window_width = button_width + 2 * padding;
	settings_window_height += button_height + (LEVEL_COUNT - 1) * line_height;
	settings_window_rect = (Rectangle){
		.x = (win_width - settings_window_width) / 2,
			.y = (win_height - settings_window_height) / 2,
			.w = settings_window_width,
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

	draw_filled_rectangle(shadowcolor, (Rectangle){.x=0, .y=0, .w=win_width, .h=win_height});
	Rectangle r = settings_window_rect;
	draw_filled_rectangle(bgcolor, r);
	float line_width = win_height / 200.0f;
	draw_rectangle(bordercolor, line_width, r);

	float padding = win_width * 0.05f;
	float y = r.y + padding;
	render_texture_with_anchor(t_difficulty, center_x, y, 0.0f, text_height_title, CENTER_H, TOP);
	y += line_height_title;
	float x = r.x + padding;
	float x_text = x + padding_button_h;
	for (int i = 0; i < LEVEL_COUNT; i++) {
		// draw button frame and background
		b_difficulty[i] = (Rectangle){x, y, button_width, button_height};
		if (i + 1 == difficulty)
			draw_highlighted_button(button_height / 20.0f, b_difficulty[i]);
		else
			draw_button(button_height / 20.0f, b_difficulty[i]);

		// draw button text
		float y_text = y + padding_button_v;
		Rectangle digit = render_texture(t_digits[i+1], x_text, y_text, 0, text_height);
		render_texture(t_levels[i], x_text + digit.w + padding / 2, y_text,
				0, text_height);
		y += line_height;
	}
}
