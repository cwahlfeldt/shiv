#ifndef SDL_FUNCTIONS_H
#define SDL_FUNCTIONS_H

#define SDL_FUNCTION_DEFS                                                                                                          \
  X(int, SDL_Init, (Uint32 flags))                                                                                                 \
  X(void, SDL_Quit, (void))                                                                                                        \
  X(void, SDL_Delay, (Uint32 ms))                                                                                                  \
  X(const char*, SDL_GetError, (void))                                                                                             \
  X(bool, SDL_SetWindowIcon, (SDL_Window * window, SDL_Surface * icon))                                                            \
  X(void, SDL_DestroySurface, (SDL_Surface * surface))                                                                             \
  X(SDL_Surface*, SDL_CreateSurfaceFrom, (void* pixels, int width, int height, int depth, int pitch, Uint32 format))               \
  X(SDL_Window*, SDL_CreateWindow, (const char* title, int w, int h, Uint32 flags))                                                \
  X(SDL_Renderer*, SDL_CreateRenderer, (SDL_Window * window, const char* name))                                                    \
  X(SDL_Texture*, SDL_CreateTexture, (SDL_Renderer * renderer, Uint32 format, int access, int w, int h))                           \
  X(void, SDL_DestroyTexture, (SDL_Texture * texture))                                                                             \
  X(void, SDL_DestroyRenderer, (SDL_Renderer * renderer))                                                                          \
  X(void, SDL_DestroyWindow, (SDL_Window * window))                                                                                \
  X(void, SDL_RenderPresent, (SDL_Renderer * renderer))                                                                            \
  X(bool, SDL_SetRenderLogicalPresentation, (SDL_Renderer * renderer, int w, int h, SDL_RendererLogicalPresentation mode))         \
  X(bool, SDL_SetRenderDrawBlendMode, (SDL_Renderer * renderer, SDL_BlendMode blend_mode))                                         \
  X(bool, SDL_SetRenderDrawColor, (SDL_Renderer * renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a))                                   \
  X(bool, SDL_SetTextureBlendMode, (SDL_Texture * texture, SDL_BlendMode blend_mode))                                              \
  X(bool, SDL_SetRenderScale, (SDL_Renderer * renderer, float scaleX, float scaleY))                                               \
  X(bool, SDL_RenderClear, (SDL_Renderer * renderer))                                                                              \
  X(bool, SDL_PollEvent, (SDL_Event * event))                                                                                      \
  X(bool, SDL_UpdateTexture, (SDL_Texture * texture, const SDL_Rect* rect, const void* pixels, int pitch))                         \
  X(bool, SDL_RenderTexture, (SDL_Renderer * renderer, SDL_Texture * texture, const SDL_FRect* srcrect, const SDL_FRect* dstrect)) \
  X(bool, SDL_UpdateWindowSurface, (SDL_Window * window))                                                                          \
  X(Uint64, SDL_GetTicks, (void))                                                                                                  \
  X(SDL_GPUCommandBuffer*, SDL_AcquireGPUCommandBuffer, (SDL_GPUDevice * a))

#endif