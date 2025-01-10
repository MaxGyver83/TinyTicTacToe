#include "pixmap.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

unsigned char *
rgb_to_rgba(const unsigned char *src, int pixel_count)
{
	unsigned char *dst = malloc(pixel_count * BYTES_PER_RGBA_PIXEL);
	for (int i = 0; i < pixel_count ; i++) {
		int i_src = i * BYTES_PER_RGB_PIXEL;
		int i_dst = i * BYTES_PER_RGBA_PIXEL;
		memcpy(&dst[i_dst], &src[i_src], 3);
		dst[i_dst + 3] = 0xFF;
	}
	return dst;
}

unsigned char *
bitmap_to_rgba(const unsigned char *src, int pixel_count)
{
	// initialize with black transparent pixels (0, 0, 0, 0)
	unsigned char *dst = calloc(pixel_count * BYTES_PER_RGBA_PIXEL, 1);
	for (int i = 0; i < pixel_count ; i++)
		if (src[i]) // black foreground/text
			dst[i * BYTES_PER_RGBA_PIXEL + 3] = src[i];
	return dst;
}

void
pixmap_copy(struct Pixmap dst, struct Pixmap src, int x, int y)
{
	assert(src.bytes_per_pixel == dst.bytes_per_pixel);
	int row_count = src.h;
	if (y + row_count > dst.h) {
		error("WARNING: pixmap_copy: row out of bounds: %d > %d", y + src.h, dst.h);
		row_count = dst.h - y;
	}
	int column_count = src.w;
	if (x + column_count > dst.w) {
		error("WARNING: pixmap_copy: column out of bounds: %d > %d", x + src.w, dst.w);
		column_count = dst.w - x;
	}
	for (int row_src = 0; row_src < row_count; row_src++) {
		int row_dst = row_src + y;
		int i_dst = (row_dst * dst.w + x) * dst.bytes_per_pixel;
		int i_src = (row_src * src.w) * src.bytes_per_pixel;
		memcpy(&dst.data[i_dst], &src.data[i_src], column_count * dst.bytes_per_pixel);
	}
}
