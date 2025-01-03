#include "mouse.h"

MouseState mouse;

void
mouse_init(MouseState *mouse)
{
	mouse->x = 0.0f;
	mouse->y = 0.0f;
	mouse->x0 = 0.0f;
	mouse->y0 = 0.0f;
	mouse->x_traveldistance = 0.0f;
	mouse->y_traveldistance = 0.0f;
	mouse->is_down = false;
	mouse->is_released = false;
	mouse->is_moved = false;
}

void
mouse_reset(MouseState *mouse)
{
	/* mouse->x = -1.0f; */
	/* mouse->y = -1.0f; */
	mouse->is_down = false;
	mouse->is_released = false;
	mouse->is_moved = false;
}

bool
is_mouse_in_rectangle(Rectangle r)
{
	return (r.x <= mouse.x && mouse.x <= r.x + r.w &&
			r.y <= mouse.y && mouse.y <= r.y + r.h);
}
