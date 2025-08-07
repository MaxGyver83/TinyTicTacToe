#include "font.h"
#include <GLES2/gl2.h>                // for GLuint, glEnableVertexAttribArray
#include "graphics.h"

typedef struct {
	float x, y;
} Point;

typedef struct {
	const Point* points; // pairs of points (2 per line)
	int count;           // number of points (must be even)
	float width;         // width of the glyph relative to height
} Glyph;

extern int32_t win_width;
extern const Color bgcolor;
extern const Color black;
extern Texture t_o;

static const Point glyph_T[] = {
	{0.0f, 0.0f}, {1.0f, 0.0f},
	{0.5f, 0.0f}, {0.5f, 1.0f},
};

static const Point glyph_I[] = {
	{0.5f, 0.0f}, {0.5f, 1.0f},
};

static const Point glyph_N[] = {
	{0.0f, 0.0f}, {0.0f, 1.0f},
	{0.0f, 0.0f}, {1.0f, 1.0f},
	{1.0f, 0.0f}, {1.0f, 1.0f},
};

static const Point glyph_Y[] = {
	{0.0f, 0.0f}, {0.5f, 0.5f},
	{1.0f, 0.0f}, {0.5f, 0.5f},
	{0.5f, 0.5f}, {0.5f, 1.0f},
};

static const Point glyph_C[] = {
	{0.7f, 0.0f}, {1.0f, 0.3f},
	{0.3f, 0.0f}, {0.7f, 0.0f},
	{0.0f, 0.3f}, {0.3f, 0.0f},
	{0.0f, 0.3f}, {0.0f, 0.7f},
	{0.0f, 0.7f}, {0.3f, 1.0f},
	{0.3f, 1.0f}, {0.7f, 1.0f},
	{0.7f, 1.0f}, {1.0f, 0.7f},
};

static const Point glyph_A[] = {
	{0.0f, 1.0f}, {0.5f, 0.0f},
	{1.0f, 1.0f}, {0.5f, 0.0f},
	{0.1f, 0.6f}, {0.9f, 0.6f},
};

static const Point glyph_E[] = {
	{-0.2f, 0.1f}, {1.0f, 0.1f},
	{0.0f, 0.525f}, {0.9f, 0.525f},
	{-0.2f, 0.95f}, {1.0f, 0.95f},
	{0.0f, 0.1f}, {0.0f, 1.0f},
};

static const Glyph font[90] = {
	['T'] = {glyph_T, sizeof(glyph_T)/sizeof(Point), 0.8f},
	['I'] = {glyph_I, sizeof(glyph_I)/sizeof(Point), 0.2f},
	['N'] = {glyph_N, sizeof(glyph_N)/sizeof(Point), 0.5f},
	['Y'] = {glyph_Y, sizeof(glyph_Y)/sizeof(Point), 0.5f},
	['C'] = {glyph_C, sizeof(glyph_C)/sizeof(Point), 0.6f},
	['A'] = {glyph_A, sizeof(glyph_A)/sizeof(Point), 0.6f},
	['E'] = {glyph_E, sizeof(glyph_E)/sizeof(Point), 0.5f},
};

static float
draw_glyph(char c, float x, float y, float height)
{
	const Glyph *g = &font[(unsigned char)c];
	if (!g || !g->points || g->count < 2)
		return 0.0f;
	float width = g->width * height;
	float strength = win_width / 80.0f;
	int i = 0;
	const Color orange = {1.0f, 0.5f, 0.0f, 1.0f};
	const Color color = (c == 'T') ? orange : black;
	while (i + 1 < g->count) {
		draw_line(color, strength,
				x + g->points[i].x * width, y + g->points[i].y * height,
				x + g->points[i+1].x * width, y + g->points[i+1].y * height);
		i += 2;
	}
	return width;
}

void
draw_text(const char *text, float x, float y, float height, float spacing)
{
	while (*text) {
		if (*text == 'O') {
			x -= spacing;
			Rectangle r = render_texture(t_o, x, y - 0.1f * height, 0, height * 1.2f);
			x += r.w + spacing / 2;
		} else if (*text == 'C') {
			x -= spacing;
			Rectangle r = render_texture(t_o, x, y - 0.1f * height, 0, height * 1.2f);
			x += r.w * 0.6f + spacing / 2;
			r.x += r.w * 0.6f;
			r.w *= 0.4f;
			// cover right part of the circle to create a "C"
			draw_filled_rectangle(bgcolor, r);
		} else {
			float width = draw_glyph(*text, x, y, height);
			x += width + ((*text == 'T') ? spacing / 2 : spacing);
		}
		text++;
	}
}
