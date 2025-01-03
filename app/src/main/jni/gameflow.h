#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

bool init_game(void);
void update_geometry(void);
void render(void);
bool update(void);
void shutdown_game(void);

#endif // GAME_H
