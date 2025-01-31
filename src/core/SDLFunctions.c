#define _COSMO_SOURCE
#include "SDLFunctions.h"

#include <stdio.h>
#include <stdlib.h>

#include "Util.h"
#include "libc/dlopen/dlfcn.h"

static void* try_find_sdl3_lib(void) {
  static const char* candidates[] = {"lib/SDL/build/libSDL3.so", "lib/SDL/build/libSDL3.so.0", "lib/SDL/build/libSDL3.so.0.2.1", "lib/SDL/build/libSDL3-0.so", "lib/SDL/build/SDL3.dll", "lib/SDL/build/libSDL3.dylib", "lib/SDL/build/SDL3"};

  void* lib = NULL;
  for (size_t i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
    if ((lib = cosmo_dlopen(candidates[i], RTLD_LAZY))) return lib;
  }

  printf("Couldn't find SDL library (%s)\n", cosmo_dlerror());
  return NULL;
}

struct sdl_syms* try_get_sdl3_syms(void) {
  void* sdl3 = try_find_sdl3_lib();
  if (!sdl3) return NULL;

  struct sdl_syms* syms = calloc(1, sizeof(*syms));
  if (!syms) return NULL;

  syms->lib = sdl3;

#define X(ret, name, args) syms->name = cosmo_dlsym(sdl3, #name);
  SDL_FUNCTION_DEFS
#undef X

  return syms;
}

void handle_events(struct game_context* ctx) {
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
}

void update_and_render(struct game_context* ctx) {
  ctx->hue += 2.0f;
  if (ctx->hue >= 360.0f) ctx->hue = 0.0f;

  struct color c = color_from_hsl(ctx->hue, 100.0f, 50.0f);
  ctx->sym->SDL_SetRenderDrawColor(ctx->renderer, (uint8_t)(c.r * 255), (uint8_t)(c.g * 255), (uint8_t)(c.b * 255), 128);
  ctx->sym->SDL_RenderClear(ctx->renderer);
  ctx->sym->SDL_RenderPresent(ctx->renderer);
}

void cleanup_sdl(struct game_context* ctx) {
  if (ctx->renderer) {
    ctx->sym->SDL_DestroyRenderer(ctx->renderer);
  }
  if (ctx->window) {
    ctx->sym->SDL_DestroyWindow(ctx->window);
  }
  if (ctx->sym) {
    ctx->sym->SDL_Quit();
    if (ctx->sym->lib) {
      cosmo_dlclose(ctx->sym->lib);
    }
    free(ctx->sym);
  }
}