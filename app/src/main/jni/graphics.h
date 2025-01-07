#ifndef TEXTURE_H
#define TEXTURE_H

#include <GLES2/gl2.h>
#include "pixmap.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

typedef unsigned char Pixel[BYTES_PER_RGBA_PIXEL];

typedef struct {
	float r, g, b, a;
} Color;

typedef struct {
	GLuint t;
	int w;
	int h;
} Texture;

typedef struct {
	int width;
	int height;
} Size;

typedef struct {
	float x, y, w, h;
} Rectangle;

typedef enum {
	ALIGN_TOP_LEFT,
	ALIGN_TOP_CENTER,
	ALIGN_TOP_RIGHT,
	ALIGN_CENTER_LEFT,
	ALIGN_CENTER,
	ALIGN_CENTER_RIGHT,
	ALIGN_BOTTOM_LEFT,
	ALIGN_BOTTOM_CENTER,
	ALIGN_BOTTOM_RIGHT,
} Alignment;

struct Pixmap create_x_pixmap(int size, const Pixel pixel);
struct Pixmap create_o_pixmap(int size, const Pixel pixel);
Texture load_texture(const char *asset_path);
Texture load_texture_from_raw_data(const unsigned char *bitmap, int width, int height);
Texture load_texture_from_pixmap_and_free_data(struct Pixmap pixmap);
Rectangle render_texture(Texture texture, float x, float y, float width, float height, Alignment align);
Color pixel_to_color(const Pixel pixel);
void draw_filled_rectangle(const Color c, Rectangle r);
void draw_rectangle(const Color c, float thickness, float x, float y, float width, float height);
void draw_rectangle_centered(const Color c, float thickness, float center_x, float center_y, float width, float height);

#endif // TEXTURE_H
