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
#include "lib/upng.h"                 // for upng_get_components, upng_get_e...
#include "utils.h"                    // for error

#define SQR(x) ((x) * (x))

extern GLuint color_program;

Texture no_texture = {0};

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

Texture
load_texture(const char *asset_path)
{
	upng_t *png;

#ifdef X11
	if (asset_path[0] == '/') {
		png = upng_new_from_file(asset_path);
	} else {
		// prepend ../assets/
		char adapted_path[256];
		snprintf(adapted_path, sizeof(adapted_path), "../assets/%s", asset_path);
		png = upng_new_from_file(adapted_path);
	}
#else
	AAsset *file;
	file = AAssetManager_open(g_app->activity->assetManager, asset_path, AASSET_MODE_BUFFER);
	if (!file) {
		error("Failed to open asset file: %s", asset_path);
		return no_texture;
	}

	unsigned char *buffer = (unsigned char*)AAsset_getBuffer(file);
	unsigned long len_file = AAsset_getLength(file);

	png = upng_new_from_bytes(buffer, len_file);
#endif

	if (png == NULL) {
		error("Error allocating space for creating PNG from file: %s", asset_path);
		AAsset_close(file);
		return no_texture;
	}
	if (upng_get_error(png) != UPNG_EOK) {
		error("Error creating PNG from file: %s (error %i)", asset_path, upng_get_error(png));
		// upng_free(png); // this crashes in upng.c:1290
		AAsset_close(file);
		return no_texture;
	}

	upng_decode(png);
	if (upng_get_error(png) != UPNG_EOK) {
		error("Error decoding PNG from file: %s (error %i)", asset_path, upng_get_error(png));
		upng_free(png);
		AAsset_close(file);
		return no_texture;
	}

	unsigned w = upng_get_width(png);
	unsigned h = upng_get_height(png);

	const unsigned char *upng_buf = upng_get_buffer(png);
	/* printf("asset_path: %s, channels: %d\n", asset_path, upng_get_components(png)); */
	unsigned char *rgba_buf = NULL;
	int channels = upng_get_components(png);
	if (channels == 3) {
		// add alpha channel
		printf("asset_path: %s, channels: %d\n", asset_path, upng_get_components(png));
		rgba_buf = rgb_to_rgba(upng_buf, w * h);
	} else if (channels == 1) {
		printf("asset_path: %s, channels: %d\n", asset_path, upng_get_components(png));
		rgba_buf = bitmap_to_rgba(upng_buf, w * h);
	} else {
		rgba_buf = (unsigned char *)upng_buf;
	}
	Texture texture = load_texture_from_raw_data(rgba_buf, w, h);

	upng_free(png);
	if (channels == 1 || channels == 3)
		free(rgba_buf);

	AAsset_close(file);

	if (!texture.t) {
		error("Error load texture '%s'", asset_path);
		return no_texture;
	}
	return texture;
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
render_texture(Texture texture, float x, float y, float width, float height, Alignment align)
{
	if (width < 1.0f && height < 1.0f)
		return (Rectangle){0};
	if (width < 1.0f)
		width = height * texture.w / texture.h;
	if (height < 1.0f)
		height = width * texture.h / texture.w;

	switch (align) {
		case ALIGN_CENTER:
		case ALIGN_TOP_CENTER:
		case ALIGN_BOTTOM_CENTER:
			x -= width / 2.0f;
			break;
		case ALIGN_TOP_RIGHT:
		case ALIGN_CENTER_RIGHT:
		case ALIGN_BOTTOM_RIGHT:
			x -= width;
			break;
		default:
			break;
	}

	switch (align) {
		case ALIGN_CENTER_LEFT:
		case ALIGN_CENTER:
		case ALIGN_CENTER_RIGHT:
			y -= height / 2.0f;
			break;
		case ALIGN_BOTTOM_LEFT:
		case ALIGN_BOTTOM_CENTER:
		case ALIGN_BOTTOM_RIGHT:
			y -= height;
			break;
		default:
			break;
	}
	render_texture_raw(texture.t, x, y, width, height);
	return (Rectangle){x, y, width, height};
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

void
draw_rectangle_centered(const Color c, float thickness,
		float center_x, float center_y, float width, float height)
{
	draw_rectangle(c, thickness,
			center_x - width/2, center_y - height/2, width, height);
}
