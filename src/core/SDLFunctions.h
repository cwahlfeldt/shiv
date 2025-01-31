#ifndef SDL_FUNCTIONS_H
#define SDL_FUNCTIONS_H

#include "SDL3/SDL.h"
#include "SDLFunctionDefs.h"

struct sdl_syms {
  void* lib;
#define X(ret, name, args) ret(*name) args;
  SDL_FUNCTION_DEFS
#undef X
};

// Game context passed between host and plugin
struct game_context {
  struct sdl_syms* sym;
  SDL_Window* window;
  SDL_Renderer* renderer;
  float hue;  // preserved across reloads
  bool running;
};

struct sdl_syms* try_get_sdl3_syms(void);
void handle_events(struct game_context* ctx);
void update_and_render(struct game_context* ctx);
void cleanup_sdl(struct game_context* ctx);

#endif
