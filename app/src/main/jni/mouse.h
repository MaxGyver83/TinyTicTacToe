#ifndef MOUSE_H
#define MOUSE_H

#include "graphics.h"
#include <stdbool.h>

typedef struct {
	float x;
	float y;
	float x0;
	float y0;
	float x_traveldistance;
	float y_traveldistance;
	bool is_down;
	bool is_released;
	bool is_moved;
} MouseState;

extern MouseState mouse;

void mouse_init(MouseState *mouse);
void mouse_reset(MouseState *mouse);
bool is_mouse_in_rectangle(Rectangle r);

#endif // MOUSE_H
