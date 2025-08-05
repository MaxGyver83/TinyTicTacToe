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
	LEFT,
	CENTER_H,
	RIGHT,
} HorizontalAnchor;

typedef enum {
	TOP,
	CENTER_V,
	BOTTOM,
} VerticalAnchor;

struct Pixmap create_x_pixmap(int size, const Pixel pixel);
struct Pixmap create_o_pixmap(int size, const Pixel pixel);
Texture load_texture(const char *asset_path);
Texture load_texture_from_raw_data(const unsigned char *bitmap, int width, int height);
Texture load_texture_from_pixmap_and_free_data(struct Pixmap pixmap);
Rectangle render_texture(Texture texture, float x, float y, float width, float height);
Rectangle render_texture_with_anchor(Texture texture, float x, float y, float width, float height, HorizontalAnchor anchor_h, VerticalAnchor anchor_v);
Color pixel_to_color(const Pixel pixel);
void draw_filled_rectangle(const Color c, Rectangle r);
void draw_rectangle(const Color color, float thickness, Rectangle r);
void draw_button(float thickness, Rectangle r);
void draw_highlighted_button(float thickness, Rectangle r);

#endif // TEXTURE_H
