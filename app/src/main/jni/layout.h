#include "graphics.h"

typedef struct Widget Widget;
struct Widget {
	float height;
	float width;
	Texture texture;
	struct Widget *next;
};

typedef struct {
	float x;
	float y;
	float height;
	float width;
	float spacing;
	Widget *widgets;
} LayoutRow;

LayoutRow * layout_row_begin(float x, float y, float width, float height, float spacing);
void layout_row_add(LayoutRow *row, Texture texture, float width, float height);
Rectangle layout_row_end(LayoutRow *row);
void layout_row_render(LayoutRow *row);
void layout_row_free_all(LayoutRow *row);
