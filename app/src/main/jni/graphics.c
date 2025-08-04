#include "graphics.h"
#include <GLES2/gl2.h>                // for GLuint, glEnableVertexAttribArray
#ifdef X11
#define AAsset_close(x)
#else
#include <android/asset_manager.h>    // for AAsset_close, AAssetManager_open
#include <android_native_app_glue.h>  // for android_app
#endif
#include <stdio.h>                    // for printf, NULL
#include <stdlib.h>                   // for calloc
#include <string.h>                   // for memcpy
#include "init.h"                     // for g_position_handle, texture_program
#include "pixmap.h"                   // for Pixmap, rgb_to_rgba, bitmap_to_...
#include "utils.h"                    // for error
#ifdef X11
#include "utils_x11.h"                // for read_file_to_buffer
#endif

#define SQR(x) ((x) * (x))

extern GLuint color_program;
extern float gap;

static const Color buttoncolor = {0.0f, 0.0f, 1.0f, 0.25f};
static const Color buttoncolor_highlighted = {1.0f, 0.5f, 0.0f, 0.75f};
static Texture no_texture = {0};

struct Pixmap
create_x_pixmap(int size, const Pixel pixel)
{
	int width = size;
	int height = size;
	float thickness = size / 10.0f;
	int byte_count = width * height * BYTES_PER_RGBA_PIXEL;
	unsigned char *buffer = calloc(byte_count, 1);
	for (int x = 0 + thickness; x < width - thickness; x++) {
		for (int distance = -thickness/2; distance < thickness/2; distance++) {
			int x2 = x - distance;
			int y2 = x + distance;
			int byte_pos = (y2 * width + x2) * BYTES_PER_RGBA_PIXEL;
			memcpy(&buffer[byte_pos], pixel, BYTES_PER_RGBA_PIXEL);

			byte_pos = (y2 * width + (width - 1 - x2)) * BYTES_PER_RGBA_PIXEL;
			memcpy(&buffer[byte_pos], pixel, BYTES_PER_RGBA_PIXEL);

			y2++;
			byte_pos = (y2 * width + x2) * BYTES_PER_RGBA_PIXEL;
			memcpy(&buffer[byte_pos], pixel, BYTES_PER_RGBA_PIXEL);

			byte_pos = (y2 * width + (width - 1 - x2)) * BYTES_PER_RGBA_PIXEL;
			memcpy(&buffer[byte_pos], pixel, BYTES_PER_RGBA_PIXEL);
		}
	}
	return (struct Pixmap){
		.w = width,
		.h = height,
		.bytes_per_pixel = BYTES_PER_RGBA_PIXEL,
		.data = buffer,
	};
}

struct Pixmap
create_o_pixmap(int size, const Pixel pixel)
{
	int width = size;
	int height = size;
	float thickness = size / 7.0f;
	float radius = size / 2.0f - thickness;
	float xcenter = width / 2.0f;
	float ycenter = height / 2.0f;
	int byte_count = width * height * BYTES_PER_RGBA_PIXEL;
	unsigned char *buffer = calloc(byte_count, 1);
	for (int x = 0; x < width; x++) {
		int relx = x - xcenter;
		for (int y = 0; y < height; y++) {
			int rely = y - ycenter;
			if (SQR(relx) + SQR(rely) >= SQR(radius - thickness/2)
					&& SQR(relx) + SQR(rely) <= SQR(radius + thickness/2)) {
				int byte_pos = (y * width + x) * BYTES_PER_RGBA_PIXEL;
				memcpy(&buffer[byte_pos], pixel, BYTES_PER_RGBA_PIXEL);
			}

		}
	}
	return (struct Pixmap){
		.w = width,
		.h = height,
		.bytes_per_pixel = BYTES_PER_RGBA_PIXEL,
		.data = buffer,
	};
}

static char*
read_sprite_file(const char *filepath)
{
	char adapted_path[256];
#ifdef X11
	// prepend ../assets/sprites/
	snprintf(adapted_path, sizeof(adapted_path), "../assets/sprites/%s", filepath);
	return read_file_to_buffer(adapted_path);
#else
	// prepend sprites/
	snprintf(adapted_path, sizeof(adapted_path), "sprites/%s", filepath);
	AAsset *file;
	file = AAssetManager_open(g_app->activity->assetManager, adapted_path, AASSET_MODE_BUFFER);
	if (!file) {
		error("Failed to open asset file: %s", adapted_path);
		return NULL;
	}
	return (char*)AAsset_getBuffer(file);
#endif
}

static Texture
load_texture_from_pgm(const char *filepath)
{
	char *buffer = read_sprite_file(filepath);
	if (!buffer)
		return no_texture;
	char *line;
	line = strtok(buffer, "\n");
	if (!line || strcmp(line, "P5") != 0)
		goto parse_error;

	line = strtok(NULL, "\n");
	if (!line)
		goto parse_error;
	int width, height;
	if (sscanf(line, "%d %d", &width, &height) != 2)
		goto parse_error;

	line = strtok(NULL, "\n");
	if (!line)
		goto parse_error;
	int maxval = 0;
	if (sscanf(line, "%d", &maxval) != 1 || maxval != 255)
		goto parse_error;

	unsigned char *img = (unsigned char *)line;
	while (*img)
		img++;
	img++;
	if (!img)
		goto parse_error;

	unsigned char *rgba_buf = bitmap_to_rgba(img, width * height);
	free(buffer);

	Texture t = load_texture_from_raw_data(rgba_buf, width, height);
	free(rgba_buf);
	return t;

parse_error:
		error("Invalid PGM file");
		free(buffer);
		return no_texture;

}

Texture
load_texture(const char *asset_path)
{
	if (endswith(".pgm", asset_path))
		return load_texture_from_pgm(asset_path);

	error("Unexpected file ending!");
	return no_texture;
}

Texture
load_texture_from_raw_data(const unsigned char *bitmap, int width, int height)
{
	GLuint texture_id = 0;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap);

	if (!texture_id) {
		error("Error while creating texture.");
		return no_texture;
	}

	return (Texture){texture_id, width, height};
}

static Texture
load_texture_from_pixmap(struct Pixmap pixmap) {
	return load_texture_from_raw_data(pixmap.data, pixmap.w, pixmap.h);
}

Texture
load_texture_from_pixmap_and_free_data(struct Pixmap pixmap) {
	Texture t = load_texture_from_pixmap(pixmap);
	free(pixmap.data);
	return t;
}

static float
normalize_x(float x)
{
	return (2.0f * x / win_width) - 1.0f;
}

static float
normalize_y(float y)
{
	return 1.0f - (2.0f * y / win_height);
}

static void
render_texture_raw(GLuint texture, float x, float y, float width, float height)
{
	glUseProgram(texture_program);

	// pixel coordinates to normal coordinates
	float normalized_left = normalize_x(x);
	float normalized_right = normalize_x(x + width);
	float normalized_top = normalize_y(y);
	float normalized_bottom = normalize_y(y + height);

	GLfloat vertices[] = {
		normalized_left, normalized_top, 0.0f, 0.0f,
		normalized_right, normalized_top, 1.0f, 0.0f,
		normalized_right, normalized_bottom, 1.0f, 1.0f,
		normalized_left, normalized_bottom, 0.0f, 1.0f
	};

	GLuint indices[] = {
		0, 1, 2,
		2, 3, 0
	};

	GLuint vbo, ebo;
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	GLint position_attrib = glGetAttribLocation(texture_program, "a_position");
	glEnableVertexAttribArray(position_attrib);
	glVertexAttribPointer(position_attrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);

	GLint tex_coord_attrib = glGetAttribLocation(texture_program, "a_tex_coord");
	glEnableVertexAttribArray(tex_coord_attrib);
	glVertexAttribPointer(tex_coord_attrib, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform1i(glGetUniformLocation(texture_program, "u_texture"), 0);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ebo);
}

Rectangle
render_texture_with_anchor(Texture texture, float x, float y, float width, float height,
		HorizontalAnchor anchor_h, VerticalAnchor anchor_v)
{
	if (width < 1.0f && height < 1.0f)
		return (Rectangle){0};
	if (width < 1.0f)
		width = height * texture.w / texture.h;
	if (height < 1.0f)
		height = width * texture.h / texture.w;

	if (anchor_h == RIGHT)
		x -= width;
	else if (anchor_h == CENTER_H)
		x -= width / 2.0f;

	if (anchor_v == BOTTOM)
		y -= height;
	else if (anchor_v == CENTER_V)
		y -= height / 2.0f;

	render_texture_raw(texture.t, x, y, width, height);
	return (Rectangle){x, y, width, height};

}

Rectangle
render_texture(Texture texture, float x, float y, float width, float height)
{
	return render_texture_with_anchor(texture, x, y, width, height, LEFT, TOP);
}

Color
pixel_to_color(const Pixel pixel)
{
	return (Color){
		.r = pixel[0] / 255.0f,
		.g = pixel[1] / 255.0f,
		.b = pixel[2] / 255.0f,
		.a = pixel[3] / 255.0f,
	};
}

static void
draw_filled_region(const Color c, float x, float y, float width, float height)
{
	float normalized_left = normalize_x(x);
	float normalized_right = normalize_x(x + width);
	float normalized_top = normalize_y(y);
	float normalized_bottom = normalize_y(y + height);

	// Line vertex data
	GLfloat vertices[] = {
		normalized_left, normalized_top,
		normalized_right, normalized_top,
		normalized_left, normalized_bottom,
		normalized_right, normalized_bottom,
	};

	glUseProgram(color_program);
	glVertexAttribPointer(g_position_handle, 2, GL_FLOAT, GL_FALSE, 0, vertices);
	glEnableVertexAttribArray(g_position_handle);
	glUniform4f(g_color_handle, c.r, c.g, c.b, c.a);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void
draw_filled_rectangle(const Color c, Rectangle r)
{
	draw_filled_region(c, r.x, r.y, r.w, r.h);
}

static void
draw_line(const Color c, float thickness, float x1, float y1, float x2, float y2)
{
	// Line vertex data
	GLfloat vertices[] = {
		normalize_x(x1),
		normalize_y(y1),
		normalize_x(x2),
		normalize_y(y2),
	};

	// Use shader program
	glUseProgram(color_program);

	// Set up vertex attribute
	glEnableVertexAttribArray(g_position_handle);
	glVertexAttribPointer(g_position_handle, 2, GL_FLOAT, GL_FALSE, 0, vertices);
	glLineWidth(thickness);

	// Set uniform values
	glUniform4f(g_position_handle, c.r, c.g, c.b, c.a);

	// Draw the line
	glDrawArrays(GL_LINES, 0, 2);
}

void
draw_rectangle(const Color color, float thickness,
		float x, float y, float width, float height)
{
	float left = x;
	float right = x + width;
	float top = y;
	float bottom = y + height;
	float t2 = thickness / 2.0f;
	draw_line(color, thickness, left - t2, top, right + t2, top);
	draw_line(color, thickness, left - t2, bottom, right + t2, bottom);
	draw_line(color, thickness, left, top, left, bottom);
	draw_line(color, thickness, right, top, right, bottom);
}

static void
draw_color_button(float thickness, Rectangle r, Color color)
{
	draw_filled_rectangle(color, r);

	float left = r.x;
	float right = r.x + r.w;
	float top = r.y;
	float bottom = r.y + r.h;
	float t2 = thickness / 2.0f;
	Color bright = {1.0f, 1.0f, 1.0f, 0.5f};
	Color dark = {0.0f, 0.0f, 0.0f, 0.5f};

	draw_line(bright, thickness, left - t2, top, right + t2, top);
	draw_line(dark, thickness, left - t2, bottom, right + t2, bottom);
	draw_line(bright, thickness, left, top, left, bottom);
	draw_line(dark, thickness, right, top, right, bottom);
}

void
draw_button(float thickness, Rectangle r)
{
	draw_color_button(thickness, r, buttoncolor);
}

void
draw_highlighted_button(float thickness, Rectangle r)
{
	draw_color_button(thickness, r, buttoncolor_highlighted);
}
