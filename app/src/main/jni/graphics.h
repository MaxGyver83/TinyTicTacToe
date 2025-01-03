#ifndef TEXTURE_H
#define TEXTURE_H

#include <GLES2/gl2.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define BYTES_PER_PIXEL 4

typedef unsigned char Pixel[BYTES_PER_PIXEL];

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

unsigned char * create_x_bitmap(int size, const Pixel color);
unsigned char * create_o_bitmap(int size, const Pixel color);
Texture load_texture(const char *asset_path);
GLuint load_texture_from_bitmap(const Pixel bitmap, int width, int height);
Rectangle render_texture(Texture texture, float x, float y, float width, float height, Alignment align);
void draw_filled_rectangle(const Pixel color, Rectangle r);
void draw_filled_region(const Pixel color, float x, float y, float width, float height);
void draw_rectangle(const Pixel color, float thickness, float x, float y, float width, float height);
void draw_rectangle_centered(const Pixel color, float thickness, float center_x, float center_y, float width, float height);

#endif // TEXTURE_H
