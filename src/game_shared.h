#ifndef GAME_SHARED_H
#define GAME_SHARED_H

#include "../modules/SDL/include/SDL3/SDL.h"

// Shared structure to pass SDL symbols
struct sdl_syms {
  void *lib;
  int (*SDL_Init)(Uint32 flags);
  void (*SDL_Quit)(void);
  void (*SDL_Delay)(Uint32 ms);
  const char *(*SDL_GetError)(void);
  bool (*SDL_SetWindowIcon)(SDL_Window *window, SDL_Surface *icon);
  void (*SDL_DestroySurface)(SDL_Surface *surface);
  SDL_Surface *(*SDL_CreateSurfaceFrom)(void *pixels, int width, int height,
                                        int depth, int pitch, Uint32 format);
  SDL_Window *(*SDL_CreateWindow)(const char *title, int w, int h,
                                  Uint32 flags);
  SDL_Renderer *(*SDL_CreateRenderer)(SDL_Window *window, const char *name);
  SDL_Texture *(*SDL_CreateTexture)(SDL_Renderer *renderer, Uint32 format,
                                    int access, int w, int h);
  void (*SDL_DestroyTexture)(SDL_Texture *texture);
  void (*SDL_DestroyRenderer)(SDL_Renderer *renderer);
  void (*SDL_DestroyWindow)(SDL_Window *window);
  void (*SDL_RenderPresent)(SDL_Renderer *renderer);
  bool (*SDL_SetRenderLogicalPresentation)(
      SDL_Renderer *renderer, int w, int h,
      SDL_RendererLogicalPresentation mode);
  bool (*SDL_SetRenderDrawBlendMode)(SDL_Renderer *renderer,
                                     SDL_BlendMode blend_mode);
  bool (*SDL_SetRenderDrawColor)(SDL_Renderer *renderer, Uint8 r, Uint8 g,
                                 Uint8 b, Uint8 a);
  bool (*SDL_SetTextureBlendMode)(SDL_Texture *texture,
                                  SDL_BlendMode blend_mode);
  bool (*SDL_SetRenderScale)(SDL_Renderer *renderer, float scaleX,
                             float scaleY);
  bool (*SDL_RenderClear)(SDL_Renderer *renderer);
  bool (*SDL_PollEvent)(SDL_Event *event);
  bool (*SDL_UpdateTexture)(SDL_Texture *texture, const SDL_Rect *rect,
                            const void *pixels, int pitch);
  bool (*SDL_RenderTexture)(SDL_Renderer *renderer, SDL_Texture *texture,
                            const SDL_FRect *srcrect, const SDL_FRect *dstrect);
  bool (*SDL_UpdateWindowSurface)(SDL_Window *window);
};

// Game context passed between host and plugin
struct game_context {
  struct sdl_syms *sym;
  SDL_Window *window;
  SDL_Renderer *renderer;
  float hue; // preserved across reloads
  bool running;
};

#endif
