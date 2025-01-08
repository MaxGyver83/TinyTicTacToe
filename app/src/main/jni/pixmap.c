#include "pixmap.h"
#include <stdlib.h>
#include <string.h>

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
			dst[i * BYTES_PER_RGBA_PIXEL + 3] = 0xFF;
	return dst;
}
