#ifndef SHIV_GAME_H
#define SHIV_GAME_H

#include "shiv_sdl.h"

// Initialize game state
struct game_context* game_init(struct sdl_syms* sym);

// Update and render one frame
void game_update(struct game_context* ctx);

// Cleanup game state
void game_cleanup(struct game_context* ctx);

#endif // SHIV_GAME_H
