#include <stdbool.h>
#include "graphics.h"
#include "pixmap.h"
#include "lib/schrift.h"

bool init_font(void);
struct Pixmap create_pixmap_from_string(char *str);
Texture create_texture_from_string(char *str);
void shutdown_font(void);
