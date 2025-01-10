#include "font.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef X11
#include <android/asset_manager.h>    // for AAsset_close, AAssetManager_open
#include <android_native_app_glue.h>  // for android_app
#endif
#include "lib/schrift.h"
#include "pixmap.h"
#include "utils.h"

#define BYTES_PER_RGBA_PIXEL 4
#define FONT "fonts/NotoSerif-Regular-subset-nohinting-nofeatures.ttf"

typedef struct Glyph {
	int width;
	int height;
	unsigned char *img;
	SFT_GMetrics metrics;
} Glyph;

extern struct android_app *g_app;

static SFT sft;
static SFT_LMetrics lmtx;

// static void
// print_metrics(void)
// {
// 	debug("LMetrics:");
// 	debug(" ascender: %.2f", lmtx.ascender);
// 	debug(" descender: %.2f", lmtx.descender);
// 	debug(" line gap: %f", lmtx.lineGap);
// }
//
// static void
// print_glyph(Glyph g)
// {
// 	debug("Glyph: %d x %d", g.width, g.height);
// 	debug("Metrics:");
// 	debug("  advance width: %.2f", g.metrics.advanceWidth);
// 	debug("  left side bearing: %.2f", g.metrics.leftSideBearing);
// 	debug("  y-offset: %d", g.metrics.yOffset);
// 	debug("  min width: %d", g.metrics.minWidth);
// 	debug("  min height: %d", g.metrics.minHeight);
// }

static bool
find_font(char *filename, unsigned int size)
{
	if (FONT[0] == '/') {
		snprintf(filename, size, "%s", FONT);
		return true;
	}
#ifdef X11
	snprintf(filename, size, "../assets/%s", FONT);
#else
	// It looks like app assets are not accessible directly in C.
	// Load file content via AAssetManager and write to local file.
	AAsset *asset = AAssetManager_open(g_app->activity->assetManager,
			FONT, AASSET_MODE_BUFFER);
	if (!asset) {
		error("Failed to open asset file: %s", FONT);
		return false;
	}
	unsigned long len_file = AAsset_getLength(asset);
	const void *buffer = AAsset_getBuffer(asset);
	debug("Font file size: %lu bytes", len_file);
	AAsset_close(asset);

	snprintf(filename, size, "%s/font.ttf", g_app->activity->internalDataPath);
	FILE *out = fopen(filename, "w");
	if (!out) {
		error("Could not open output file: %s", filename);
		return false;
	}
	fwrite(buffer, len_file, 1, out);
	fclose(out);
#endif
	return true;
}

bool
init_font(void)
{
	char local_font[256];
	if (!find_font(local_font, sizeof(local_font)))
		return false;
	sft.font = sft_loadfile(local_font);
	if (sft.font == NULL) {
		error("TTF load failed. Could not load %s!", local_font);
		return false;
	}

	sft.xScale = sft.yScale = 32;
	sft.flags = SFT_DOWNWARD_Y;
	sft_lmetrics(&sft, &lmtx);
	// print_metrics();
	return true;
}

static Glyph
render_glyph(unsigned long cp)
{
	SFT_Glyph gid;  //  unsigned long gid;
	if (sft_lookup(&sft, cp, &gid) < 0) {
		error("codepoint 0x%04lX missing", cp);
		return (Glyph){0};
	}

	SFT_GMetrics mtx;
	if (sft_gmetrics(&sft, gid, &mtx) < 0) {
		error("codepoint 0x%04lX bad glyph metrics", cp);
		return (Glyph){0};
	}

	SFT_Image img = {
		// Round up to multiple of 4?
		// .width  = (mtx.minWidth + 3) & ~3,
		// Or round up to 2^x?
		.width  = mtx.minWidth,
		.height = mtx.minHeight,
	};
	unsigned char *bitmap = malloc(img.width * img.height);
	img.pixels = bitmap;
	if (sft_render(&sft, gid, img) < 0) {
		error("codepoint 0x%04lX not rendered", cp);
		return (Glyph){0};
	}
	return (Glyph) {
		.width = img.width,
		.height = img.height,
		.img = img.pixels,
		.metrics = mtx,
	};
}

struct Pixmap
create_pixmap_from_string(char *str)
{
	int len = strlen(str);
	Glyph glyphs[len];
	for (int i = 0; i < len; i++) {
		// convert str[i] to UTF-32 for non-ASCII characters
		glyphs[i] = render_glyph(str[i]);
		// debug("\nbuffers[%d], char: %c", i, str[i]);
		// print_glyph(glyphs[i]);
	}

	// calculate size of joined buffer and create it
	int last = len - 1;
	float x = 0.0f;
	for (int i = 0; i < last; i++)
		x += glyphs[i].metrics.advanceWidth;
	int width = (int)(x + glyphs[last].metrics.leftSideBearing) + glyphs[last].width;
	int height = (int)(lmtx.ascender - lmtx.descender + 1);
	int size = width * height;
	struct Pixmap str_img = {
		.w = width,
		.h = height,
		.bytes_per_pixel = 1,
		.data = calloc(size, 1),
	};

	x = 0.0f;
	for (int i = 0; i < len; i++) {
		struct Pixmap glyph_img = {
			.w = glyphs[i].width,
			.h = glyphs[i].height,
			.bytes_per_pixel = 1,
			.data = glyphs[i].img,
		};
		int x_pos = (int)(x + glyphs[i].metrics.leftSideBearing);
		/* int y_pos = (int)height + glyphs[i].metrics.yOffset; */
		int y_pos = (int)(lmtx.ascender) + glyphs[i].metrics.yOffset;
		pixmap_copy(str_img, glyph_img, x_pos, y_pos);
		x += glyphs[i].metrics.advanceWidth;
	}

	unsigned char *tmp = str_img.data;
	str_img.data = bitmap_to_rgba(str_img.data, size);
	str_img.bytes_per_pixel = BYTES_PER_RGBA_PIXEL;
	free(tmp);
	return str_img;
}

Texture
create_texture_from_string(char *str)
{
	struct Pixmap pixmap = create_pixmap_from_string(str);
	return load_texture_from_pixmap_and_free_data(pixmap);
}

void
shutdown_font()
{
	sft_freefont(sft.font);
}
