#include <stdio.h>
#include <stdlib.h>

#define _COSMO_SOURCE

#include "core/SDLFunctions.h"

#define WIDTH 1280
#define HEIGHT 800

int main(void) {
  struct game_context ctx = {0};

  // Initialize SDL systems
  ctx.sym = try_get_sdl3_syms();
  if (!ctx.sym) {
    printf("Failed to get symbols\n");
    return -1;
  }

  if (!ctx.sym->SDL_Init(SDL_INIT_VIDEO)) {
    printf("SDL couldn't initialize: %s\n", ctx.sym->SDL_GetError());
    return -1;
  }

  uint32_t flags = SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE;
  ctx.window = ctx.sym->SDL_CreateWindow("shiv © Christopher C Wahlfeldt, press Q or click on window to exit", WIDTH, HEIGHT, flags);

  if (!ctx.window) {
    printf("Window couldn't be created: %s\n", ctx.sym->SDL_GetError());
    return -1;
  }

  ctx.renderer = ctx.sym->SDL_CreateRenderer(ctx.window, NULL);
  if (!ctx.renderer) {
    printf("Renderer couldn't be created: %s\n", ctx.sym->SDL_GetError());
    ctx.sym->SDL_DestroyWindow(ctx.window);
    return -1;
  }

  // Initialize rendering settings
  ctx.sym->SDL_SetRenderLogicalPresentation(ctx.renderer, WIDTH, HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);
  ctx.sym->SDL_SetRenderDrawBlendMode(ctx.renderer, SDL_BLENDMODE_BLEND);
  ctx.sym->SDL_SetRenderScale(ctx.renderer, 1.0f, 1.0f);

  ctx.hue = 0.0f;
  ctx.running = true;

  // Main game loop
  while (ctx.running) {
    handle_events(&ctx);
    update_and_render(&ctx);
    ctx.sym->SDL_Delay(16);  // ~60 FPS
  }

  cleanup_sdl(&ctx);
  return 0;
}