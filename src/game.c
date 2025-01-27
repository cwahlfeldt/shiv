#include "game.h"
#include "shiv_color.h"
#include <stdio.h>
#include <stdlib.h>

#define WIDTH 1280
#define HEIGHT 800

struct game_context *game_init(struct sdl_syms *sym) {
  struct game_context *ctx = calloc(1, sizeof(struct game_context));
  if (!ctx)
    return NULL;

  ctx->sym = sym;

  uint32_t flags = SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE;
  ctx->window = sym->SDL_CreateWindow(
      "shiv © Christopher C Wahlfeldt, press Q or click on window to exit",
      WIDTH, HEIGHT, flags);

  if (!ctx->window) {
    printf("Window couldn't be created, error: \"%s\"\n", sym->SDL_GetError());
    free(ctx);
    return NULL;
  }

  ctx->renderer = sym->SDL_CreateRenderer(ctx->window, NULL);
  if (!ctx->renderer) {
    printf("Renderer couldn't be created, error: \"%s\"\n",
           sym->SDL_GetError());
    sym->SDL_DestroyWindow(ctx->window);
    free(ctx);
    return NULL;
  }

  sym->SDL_SetRenderLogicalPresentation(ctx->renderer, WIDTH, HEIGHT,
                                        SDL_LOGICAL_PRESENTATION_LETTERBOX);
  sym->SDL_SetRenderDrawBlendMode(ctx->renderer, SDL_BLENDMODE_BLEND);
  sym->SDL_SetRenderScale(ctx->renderer, 1.0f, 1.0f);

  ctx->hue = 0.0f;
  ctx->running = true;

  return ctx;
}

void game_update(struct game_context *ctx) {
  SDL_Event event = {0};

  while (ctx->sym->SDL_PollEvent(&event)) {
    if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0) {
      if (event.key.scancode == SDL_SCANCODE_Q) {
        ctx->running = false;
      }
    }
    if (event.type == SDL_EVENT_QUIT) {
      ctx->running = false;
    }
  }

  ctx->hue += 0.5f;
  if (ctx->hue >= 360.0f)
    ctx->hue = 0.0f;

  struct color c = color_from_hsl(ctx->hue, 100, 50);

  ctx->sym->SDL_SetRenderDrawColor(ctx->renderer, c.r * 255, c.g * 255,
                                   c.b * 255, 128);
  ctx->sym->SDL_RenderClear(ctx->renderer);
  ctx->sym->SDL_RenderPresent(ctx->renderer);
  ctx->sym->SDL_UpdateWindowSurface(ctx->window);
  ctx->sym->SDL_Delay(16);
}

void game_cleanup(struct game_context *ctx) {
  if (!ctx)
    return;

  if (ctx->renderer) {
    ctx->sym->SDL_DestroyRenderer(ctx->renderer);
  }
  if (ctx->window) {
    ctx->sym->SDL_DestroyWindow(ctx->window);
  }

  free(ctx);
}
