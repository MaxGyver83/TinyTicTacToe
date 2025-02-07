#include "layout.h"

#include <stdlib.h>

LayoutRow *
layout_row_begin(float x, float y, float width, float height, float spacing)
{
	// Either width or spacing must be 0.
	LayoutRow *row = malloc(sizeof(LayoutRow));
	*row = (LayoutRow){
		.x=x, .y=y, .width=width, .height=height, .spacing=spacing
	};
	return row;
}

void
layout_row_add(LayoutRow *row, Texture texture, float width, float height)
{
	if (height < 1.0f)
		height = row->height;
	if (width < 1.0f)
		width = height / texture.h * texture.w;
	Widget *widget = malloc(sizeof(Widget));
	*widget = (Widget){.texture=texture, .height=height, .width=width};

	if (row->widgets) {
		Widget *last = row->widgets;
		while (last->next)
			last = last->next;
		last->next = widget;
	} else {
		row->widgets = widget;
	}
}

Rectangle
layout_row_end(LayoutRow *row)
{
	if (!row)
		return (Rectangle){0};
	int widget_count = 0;
	float width_sum = 0.0f;
	if (row->widgets) {
		widget_count = 1;
		width_sum = row->widgets->width;
		Widget *w = row->widgets;
		while ((w = w->next)) {
			width_sum += row->spacing + w->width;
			widget_count++;
		}
		if (row->width == 0.0f) {
			row->width = width_sum;
		} else {
			if (widget_count == 1)
				row->spacing = 0.0f;
			else
				row->spacing = (row->width - width_sum) / (widget_count - 1);
		}
	}
	return (Rectangle){row->x, row->y, row->width, row->height};
}

void
layout_row_render(LayoutRow *row)
{
	if (!row)
		return;
	float x = row->x;
	Widget *widget = row->widgets;
	while (widget) {
		Rectangle r = render_texture(
				widget->texture,
				x,
				row->y,
				widget->width,
				widget->height);
		x += r.w + row->spacing;
		widget = widget->next;
	};
}

void
layout_row_free_all(LayoutRow *row)
{
	if (!row)
		return;
	Widget *widget = row->widgets;
	while (widget) {
		Widget *next = widget->next;
		free(widget);
		widget = next;
	};
	free(row);
}
