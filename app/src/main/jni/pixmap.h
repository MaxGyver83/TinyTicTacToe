#ifndef PIXMAP_H
#define PIXMAP_H

#define BYTES_PER_RGB_PIXEL 3
#define BYTES_PER_RGBA_PIXEL 4

// No typedef here because X.h (used in main_x11.c) defines a Pixmap type.
struct Pixmap {
	int w; // in pixels
	int h; // in pixels;
	int bytes_per_pixel;
	unsigned char *data;
};

unsigned char * rgb_to_rgba(const unsigned char *src, int pixel_count);
unsigned char * bitmap_to_rgba(const unsigned char *src, int pixel_count);

#endif // PIXMAP_H
