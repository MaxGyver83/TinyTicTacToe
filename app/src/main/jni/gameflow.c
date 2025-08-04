#include "gameflow.h"
#include <GLES2/gl2.h>                // for glClear, glClearColor, glDelete...
#include <stdio.h>                    // for printf, fprintf, fscanf, fclose
#include <stdlib.h>                   // for free, malloc, srand
#include <string.h>                   // for memset
#include <time.h>                     // for time
#include "audio.h"                    // for destroy_audio_engine, destroy_a...
#include "gamelogic.h"                // for Line, Player, computer_move_ver...
#include "graphics.h"                 // for render_texture, load_texture
#include "init.h"                     // for win_width, win_height
#include "layout.h"                   // for layout_row_add, layout_row_begin
#include "mouse.h"                    // for mouse, is_mouse_in_rectangle
#include "settings.h"                 // for render_window
#include "utils.h"                    // for error, info, random_int, sleep_...

#define LEVEL_COUNT 5

// variables
extern char stats_filepath[256];
extern bool show_settings;
extern int winning_line;
extern int winner;
extern int fields[FIELD_COUNT];
extern Rectangle settings_window_rect;
extern Rectangle b_difficulty[LEVEL_COUNT];

int difficulty = 1;
int keyboard_field_selection = -1;
float gap; // space between words

static const Pixel color_x = {0xFF, 0x80, 0x00, 0xFF};
static const Pixel color_o = {0x00, 0x00, 0xFF, 0xFF};
static const Pixel color_x_highlight = {0xFF, 0x80, 0x00, 0x40};
static const Pixel color_o_highlight = {0x00, 0x00, 0xFF, 0x40};
const Color bgcolor = {0.4f, 1.0f, 0.8f, 1.0f};

static int stats[] = {0, 0, 0};
static bool done = false;
static bool players_turn;
static float text_height;
static float line_height; // space between two baselines
static float padding;
static Rectangle game_area;
static Rectangle b_settings;

// sprites
Texture t_difficulty;
Texture t_digits[10];
Texture t_levels[LEVEL_COUNT];

static Texture t_grid;
static Texture t_x;
static Texture t_o;
static Texture t_x_highlight;
static Texture t_o_highlight;
static Texture t_tinytictactoe;
static Texture t_nextturn;
static Texture t_haswon;
static Texture t_draw;
static Texture *stats_textures[3] = {&t_draw, &t_x, &t_o};

static Texture
create_grid(int size)
{
	int thickness = 5;
	size_t bufsize = size * size * BYTES_PER_RGBA_PIXEL;
	unsigned char *buf = malloc(bufsize);
	memset(buf, 255, bufsize);
	for (int row = 0; row < size; row++) {
		for (int col = 0; col < size; col++) {
			if (row % (size / 3) < thickness || row >= size - thickness
					|| col % (size / 3) < thickness || col >= size - thickness) {
				int pixel_index = row * size + col;
				// black pixel {0, 0, 0}, alpha keeps default value of 255
				memset(&buf[pixel_index * BYTES_PER_RGBA_PIXEL], 0, 3);
			}
		}
	}
	Texture t = load_texture_from_raw_data(buf, (unsigned)size, (unsigned)size);
	free(buf);
	return t;
}

static void
load_statistics(void)
{
	debug("Reading %s", stats_filepath);

	FILE *file = fopen(stats_filepath, "r");
	if (!file) {
		info("Failed to open file %s. This is expected if this is the first game.", stats_filepath);
		return;
	}
	int difficulty_read = 0, draws = 0, wins = 0, losses = 0;
	fscanf(file, "%d", &difficulty_read);
	fscanf(file, "%d", &draws);
	fscanf(file, "%d", &wins);
	fscanf(file, "%d", &losses);
	fclose(file);

	if (difficulty_read >= 1 && difficulty_read <= LEVEL_COUNT)
		difficulty = difficulty_read;
	else
		info("Invalid difficulty in stats file: %d", difficulty_read);
	stats[0] = draws;
	stats[1] = wins;
	stats[2] = losses;
}

static void
save_statistics(void)
{
	debug("Writing to %s", stats_filepath);
	FILE *file = fopen(stats_filepath, "w");
	if (!file) {
		error("Failed to open file %s", stats_filepath);
		return;
	}
	fprintf(file, "%d\n", difficulty);
	fprintf(file, "%d\n", stats[0]);
	fprintf(file, "%d\n", stats[1]);
	fprintf(file, "%d\n", stats[2]);
	fclose(file);
}

bool
init_game()
{
	srand(time(NULL));
	t_grid = create_grid(win_width * 0.9f);
	t_x_highlight = load_texture_from_raw_data(color_x_highlight, 1, 1);
	t_o_highlight = load_texture_from_raw_data(color_o_highlight, 1, 1);

	// sprites
	char filename[14];
	for (int i = 0; i < 10; i++) {
		snprintf(filename, sizeof(filename), "sprites/%d.pgm", i);
		t_digits[i] = load_texture(filename);
		if (!t_digits[i].t)
			return false;
	}

	struct Pixmap pixmap = create_x_pixmap(100, color_x);
	t_x = load_texture_from_pixmap_and_free_data(pixmap);

	pixmap = create_o_pixmap(200, color_o);
	t_o = load_texture_from_pixmap_and_free_data(pixmap);

	t_tinytictactoe = load_texture("sprites/tiny_tic_tac_toe.pgm");
	t_nextturn = load_texture("sprites/next_turn.pgm");
	t_haswon = load_texture("sprites/has_won.pgm");
	t_draw = load_texture("sprites/draw.pgm");
	t_difficulty = load_texture("sprites/level.pgm");
	t_levels[0] = load_texture("sprites/very_easy.pgm");
	t_levels[1] = load_texture("sprites/easy.pgm");
	t_levels[2] = load_texture("sprites/medium.pgm");
	t_levels[3] = load_texture("sprites/hard.pgm");
	t_levels[4] = load_texture("sprites/very_hard.pgm");

	load_statistics();

	return true;
}

static void
reset_game(void)
{
	done = false;
	winner = 0;
	winning_line = -1;
	memset(fields, 0, sizeof(fields));
}

static void
render_highlight(void)
{
	if (!winner)
		return;
	float box_size = game_area.w / 3.0f;
	Texture t_highlight = (winner == X) ? t_x_highlight : t_o_highlight;
	/* Pixel *color = (winner == X) ? color_x_highlight : color_o_highlight; */
	int line_index;
	switch (winning_line) {
	case LEFT_COLUMN:
	case CENTER_COLUMN:
	case RIGHT_COLUMN:
		line_index = winning_line - LEFT_COLUMN;
		render_texture(t_highlight,
				game_area.x + line_index * box_size,
				game_area.y,
				box_size,
				game_area.h);
		/* draw_filled_region(pixel_to_color(color), */
		/*         game_area.x + line_index * box_size, */
		/*         game_area.y, */
		/*         box_size, */
		/*         game_area.h); */
		break;
	case TOP_ROW:
	case CENTER_ROW:
	case BOTTOM_ROW:
		line_index = winning_line - TOP_ROW;
		render_texture(t_highlight,
				game_area.x,
				game_area.y + line_index * box_size,
				game_area.w,
				box_size);
		break;
	case DESCENDING_DIAGONAL:
	case ASCENDING_DIAGONAL:
		line_index = winning_line - DESCENDING_DIAGONAL;
		for (int i = 0; i < 3; i++) {
			int col = line_index == 0 ? i : 2 - i;
			render_texture(t_highlight,
					game_area.x + col * box_size,
					game_area.y + i * box_size,
					box_size,
					box_size);
		}
		break;
	}
}

static void
render_xs_and_os(void)
{
	for (int row = 0; row < 3; row++) {
		for (int col = 0; col < 3; col++) {
			float field_size = game_area.w / 3.0f;
			float margin = field_size * 0.05f;
			int field = fields[row * 3 + col];
			if (field)
				render_texture_with_anchor(field == X ? t_x : t_o,
						game_area.x + col * field_size + field_size / 2.0f,
						game_area.y + row * field_size + field_size / 2.0f,
						field_size - 2 * margin,
						field_size - 2 * margin,
						CENTER_H, CENTER_V);
		}
	}
}

static void
render_winner(void)
{
	float size = game_area.w / 10.0f;
	gap = 0.15f * size;
	float y = game_area.y + game_area.h;
	y = (win_height + y - size) / 2.0f;
	if (winner) {
		float text_width = size + gap + (size * t_haswon.w / t_haswon.h);
		float x = (win_width - text_width) / 2.0f;
		render_texture(winner == X ? t_x : t_o, x, y, size, size);
		render_texture(t_haswon, x + size + gap, y, 0.0f, size);
	} else {
		// draw game
		float x = win_width / 2.0f;
		render_texture_with_anchor(t_draw, x, y, 0.0f, size, CENTER_H, TOP);
	}
}

static void
render_statistics(float right, float top)
{
	float y = top;
	float max_width = 0.0f;
	for (int i = 0; i < 3; i++) {
		float width = i ? text_height : text_height * stats_textures[i]->w / stats_textures[i]->h;
		int number = stats[i];
		do {
			int digit = number % 10;
			number = number / 10;
			float digit_width = text_height * t_digits[digit].w / t_digits[digit].h;
			width += gap + digit_width;
		} while (number > 0);
		if (width > max_width)
			max_width = width;
	}
	float left = right - max_width;
	for (int i = 0; i < 3; i++) {
		render_texture(*stats_textures[i], left, y, 0.0f, text_height);
		int number = stats[i];
		int x = right;
		do {
			int digit = number % 10;
			number = number / 10;
			Rectangle r = render_texture(t_digits[digit], x, y, 0.0f, text_height);
			x -= r.w;
		} while (number > 0);
		y += line_height;
	}
}

static Rectangle
make_button(Rectangle r)
{
	r.w += gap * 2;
	r.x -= gap;
	r.h += gap;
	r.y -= gap / 2;
	return r;
}

static void
render_game_information(void)
{
	float left = game_area.x;
	float top = game_area.y + game_area.h + padding;
	gap = game_area.w * 0.015f;
	// next turn
	Rectangle r = render_texture(t_nextturn, left, top, 0.0f, text_height);
	left += r.w + gap;
	render_texture(players_turn ? t_x : t_o, left, top, 0.0f, text_height);

	// statistics
	render_statistics(game_area.x + game_area.h, top);

	// settings button
	top += line_height * 2.0f - gap / 2.0f;
	LayoutRow *row = layout_row_begin(game_area.x + gap, top, 0.0f, text_height, gap);
	layout_row_add(row, t_difficulty, 0.0f, 0.0f);
	layout_row_add(row, t_digits[difficulty], 0.0f, 0.0f);
	b_settings = layout_row_end(row);
	layout_row_render(row);
	layout_row_free_all(row);
	draw_button(line_height / 20.0f, make_button(b_settings));
}

void
update_geometry()
{
	float short_side = MIN(win_width, win_height);
	padding = short_side * 0.05f;
	float space_for_title = padding * 2.0f; // includes padding
	game_area.w = game_area.h = short_side - padding * 2.0f;
	game_area.y = (win_height - game_area.h) / 2.0f;
	text_height = game_area.w / 14.0f;
	line_height = text_height * 1.2f;
	// try to center the game area vertically,
	// except when the stats area needs more space:
	float min_space_for_stats = padding * 2.0f + line_height * 2.0f + text_height;
	float space_for_stats = MAX(min_space_for_stats, game_area.y);
	game_area.y = win_height - space_for_stats - game_area.h;
	if (game_area.y < space_for_title) {
		game_area.y = space_for_title;
		game_area.w = game_area.h = win_height - space_for_title - space_for_stats;
	}
	game_area.x = (win_width - game_area.w) / 2.0f;
}

void
render()
{
	glViewport(0, 0, win_width, win_height);
	glClearColor(bgcolor.r, bgcolor.g, bgcolor.b, bgcolor.a);
	glClear(GL_COLOR_BUFFER_BIT);

	float text_width = game_area.w * 0.7f;
	render_texture_with_anchor(t_tinytictactoe, win_width / 2.0f, game_area.y / 2.0f, text_width, 0.0f, CENTER_H, CENTER_V);

	render_texture(t_grid, game_area.x, game_area.y, game_area.w, game_area.h);

	if (done)
		render_highlight();

	render_xs_and_os();

	if (done)
		render_winner();
	else
		render_game_information();

	if (show_settings)
		render_window();
}

static int
get_clicked_field(float mousex, float mousey, Rectangle r)
{
	float relx = mousex - r.x;
	float rely = mousey - r.y;
	int col = (int)(relx / r.w * 3);
	int row = (int)(rely / r.h * 3);
	int field = row * 3 + col;
	return field;
}

static void
check_if_done(void)
{
	done = is_done();
	if (done) {
		if (winner)
			info("Player %s has won!", winner == X ? "X" : "O");
		else
			info("Draw!");
		stats[winner]++;
		info("Stats: X: %d, O: %d, Draw: %d", stats[X], stats[O], stats[NONE]);
		save_statistics();
	}
}

static void
move_done(void)
{
	play_audio("audio/pick.ogg");
	players_turn = !players_turn;
	check_if_done();
}

static void
computer_move(void)
{
	switch (difficulty) {
	case 1: computer_move_very_easy(); break;
	case 2: computer_move_easy(); break;
	case 3: computer_move_medium(); break;
	case 4: computer_move_hard(); break;
	case 5: computer_move_flawless(); break;
	default:
		difficulty = 1; computer_move_very_easy(); break;
	}
	move_done();
}

static void
player_move(void)
{
	int field = get_clicked_field(mouse.x, mouse.y, game_area);
	if (fields[field])
		return;
	fields[field] = X;
	move_done();
}

bool
update()
{
	if (done) {
		if (mouse.is_down) {
			reset_game();
			mouse.is_down = false;
			return true;
		}
		return false;
	}

	if (!done && !players_turn) {
		sleep_milliseconds(random_int(100, 400));
		computer_move();
		return true;
	} else if (keyboard_field_selection >= 0) {
		if (!show_settings) {
			if (fields[keyboard_field_selection] == NONE) {
				fields[keyboard_field_selection] = X;
				move_done();
			}
		}
		keyboard_field_selection = -1;
		return true;
	} else if (mouse.is_down) {
		mouse.is_down = false;
		if (show_settings) {
			if (!is_mouse_in_rectangle(settings_window_rect)) {
				show_settings = false;
			} else for (int i = 0; i < LEVEL_COUNT; i++) {
				if (is_mouse_in_rectangle(b_difficulty[i])) {
					difficulty = i + 1;
					show_settings = false;
					save_statistics();
				}
			}
		} else {
			if (is_mouse_in_rectangle(game_area)) {
				player_move();
			} else if (is_mouse_in_rectangle(b_settings)) {
				show_settings = true;
			}
		}
		return true;
	}
	return false;
}

void
shutdown_game()
{
	destroy_audio_player();
	destroy_audio_engine();

	for (int i = 0; i < 10; i++)
		glDeleteTextures(1, &t_digits[i].t);
	for (int i = 0; i < LEVEL_COUNT; i++)
		glDeleteTextures(1, &t_levels[i].t);
	glDeleteTextures(1, &t_grid.t);
	glDeleteTextures(1, &t_x.t);
	glDeleteTextures(1, &t_o.t);
	glDeleteTextures(1, &t_x_highlight.t);
	glDeleteTextures(1, &t_o_highlight.t);
	glDeleteTextures(1, &t_tinytictactoe.t);
	glDeleteTextures(1, &t_nextturn.t);
	glDeleteTextures(1, &t_haswon.t);
	glDeleteTextures(1, &t_draw.t);
	glDeleteTextures(1, &t_difficulty.t);
}
