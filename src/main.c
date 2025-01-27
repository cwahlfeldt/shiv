#define _COSMO_SOURCE
#include "libc/dlopen/dlfcn.h"
#include <stdio.h>
#include <stdlib.h>
#include "shiv_color.h"
#include "shiv_sdl.h"

#define WIDTH 1280
#define HEIGHT 800

static void *try_find_sdl3_lib(void) {
  char *candidates[] = {"modules/SDL/build/libSDL3.so",
                       "modules/SDL/build/libSDL3.so.0",
                       "modules/SDL/build/libSDL3.so.0.2.1",
                       "modules/SDL/build/libSDL3-0.so",
                       "modules/SDL/build/SDL3.dll",
                       "modules/SDL/build/libSDL3.dylib",
                       "modules/SDL/build/SDL3"};

  void *lib = NULL;
  for (size_t i = 0; i < (sizeof(candidates) / sizeof(*candidates)); ++i) {
    if ((lib = cosmo_dlopen(candidates[i], RTLD_LAZY)))
      return lib;
  }

  printf("Couldn't find SDL library (%s)\n", cosmo_dlerror());
  return NULL;
}

static struct sdl_syms *try_get_sdl3_syms(void) {
  void *sdl3 = try_find_sdl3_lib();
  if (!sdl3) return NULL;

  struct sdl_syms *syms = calloc(1, sizeof(*syms));
  void *init_sym = cosmo_dlsym(sdl3, "SDL_Init");

  *syms = (struct sdl_syms){
      .lib = sdl3,
      .SDL_Init = init_sym,
      .SDL_Quit = cosmo_dlsym(sdl3, "SDL_Quit"),
      .SDL_Delay = cosmo_dlsym(sdl3, "SDL_Delay"),
      .SDL_GetError = cosmo_dlsym(sdl3, "SDL_GetError"),
      .SDL_SetWindowIcon = cosmo_dlsym(sdl3, "SDL_SetWindowIcon"),
      .SDL_DestroySurface = cosmo_dlsym(sdl3, "SDL_DestroySurface"),
      .SDL_CreateSurfaceFrom = cosmo_dlsym(sdl3, "SDL_CreateSurfaceFrom"),
      .SDL_CreateWindow = cosmo_dlsym(sdl3, "SDL_CreateWindow"),
      .SDL_CreateRenderer = cosmo_dlsym(sdl3, "SDL_CreateRenderer"),
      .SDL_CreateTexture = cosmo_dlsym(sdl3, "SDL_CreateTexture"),
      .SDL_DestroyTexture = cosmo_dlsym(sdl3, "SDL_DestroyTexture"),
      .SDL_DestroyRenderer = cosmo_dlsym(sdl3, "SDL_DestroyRenderer"),
      .SDL_DestroyWindow = cosmo_dlsym(sdl3, "SDL_DestroyWindow"),
      .SDL_RenderPresent = cosmo_dlsym(sdl3, "SDL_RenderPresent"),
      .SDL_SetRenderLogicalPresentation =
          cosmo_dlsym(sdl3, "SDL_SetRenderLogicalPresentation"),
      .SDL_SetRenderDrawBlendMode =
          cosmo_dlsym(sdl3, "SDL_SetRenderDrawBlendMode"),
      .SDL_SetRenderDrawColor = cosmo_dlsym(sdl3, "SDL_SetRenderDrawColor"),
      .SDL_SetTextureBlendMode = cosmo_dlsym(sdl3, "SDL_SetTextureBlendMode"),
      .SDL_SetRenderScale = cosmo_dlsym(sdl3, "SDL_SetRenderScale"),
      .SDL_RenderClear = cosmo_dlsym(sdl3, "SDL_RenderClear"),
      .SDL_PollEvent = cosmo_dlsym(sdl3, "SDL_PollEvent"),
      .SDL_UpdateTexture = cosmo_dlsym(sdl3, "SDL_UpdateTexture"),
      .SDL_RenderTexture = cosmo_dlsym(sdl3, "SDL_RenderTexture"),
      .SDL_UpdateWindowSurface = cosmo_dlsym(sdl3, "SDL_UpdateWindowSurface"),
      .SDL_GetTicks = cosmo_dlsym(sdl3, "SDL_GetTicks"),
  };

  return syms;
}

int main(void) {
  struct sdl_syms *sym = try_get_sdl3_syms();
  if (!sym) {
    printf("Failed to get symbols\n");
    return -1;
  }

  if (!sym->SDL_Init(SDL_INIT_VIDEO)) {
    printf("SDL couldn't initialize: %s\n", sym->SDL_GetError());
    return -1;
  }

  uint32_t flags = SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE;
  SDL_Window *window = sym->SDL_CreateWindow(
      "shiv © Christopher C Wahlfeldt, press Q or click on window to exit",
      WIDTH, HEIGHT, flags);

  if (!window) {
    printf("Window couldn't be created: %s\n", sym->SDL_GetError());
    return -1;
  }

  SDL_Renderer *renderer = sym->SDL_CreateRenderer(window, NULL);
  if (!renderer) {
    printf("Renderer couldn't be created: %s\n", sym->SDL_GetError());
    sym->SDL_DestroyWindow(window);
    return -1;
  }

  sym->SDL_SetRenderLogicalPresentation(renderer, WIDTH, HEIGHT,
                                      SDL_LOGICAL_PRESENTATION_LETTERBOX);
  sym->SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  sym->SDL_SetRenderScale(renderer, 1.0f, 1.0f);

  float hue = 0.0f;
  bool running = true;

  while (running) {
    SDL_Event event = {0};
    while (sym->SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0) {
        if (event.key.scancode == SDL_SCANCODE_Q) {
          running = false;
        }
      }
      if (event.type == SDL_EVENT_QUIT) {
        running = false;
      }
    }

    hue += 2.0f;
    if (hue >= 360.0f) hue = 0.0f;

    struct color c = color_from_hsl(hue, 100.0f, 50.0f);
    sym->SDL_SetRenderDrawColor(renderer, 
                               (uint8_t)(c.r * 255),
                               (uint8_t)(c.g * 255),
                               (uint8_t)(c.b * 255), 
                               128);
    sym->SDL_RenderClear(renderer);
    sym->SDL_RenderPresent(renderer);

    sym->SDL_Delay(16); // ~60 FPS
  }

  sym->SDL_DestroyRenderer(renderer);
  sym->SDL_DestroyWindow(window);
  sym->SDL_Quit();
  free(sym);

  return 0;
}