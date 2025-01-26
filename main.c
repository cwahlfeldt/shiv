#include <stdbool.h>

#define _COSMO_SOURCE
#include "libc/dlopen/dlfcn.h"

#include "SDL/include/SDL3/SDL.h"

// TODO: Add all SDL functions
struct sdl_syms
{
  void *lib;
  int (*SDL_Init)(Uint32 flags);
  void (*SDL_Quit)(void);
  void (*SDL_Delay)(Uint32 ms);
  const char *(*SDL_GetError)(void);
  bool (*SDL_SetWindowIcon)(SDL_Window *window, SDL_Surface *icon);
  void (*SDL_DestroySurface)(SDL_Surface *surface);
  SDL_Surface *(*SDL_CreateSurfaceFrom)(void *pixels, int width, int height,
                                        int depth, int pitch, Uint32 format);
  SDL_Window *(*SDL_CreateWindow)(const char *title, int w, int h, Uint32 flags);
  SDL_Renderer *(*SDL_CreateRenderer)(SDL_Window *window, const char *name);
  SDL_Texture *(*SDL_CreateTexture)(SDL_Renderer *renderer, Uint32 format,
                                    int access, int w, int h);
  void (*SDL_DestroyTexture)(SDL_Texture *texture);
  void (*SDL_DestroyRenderer)(SDL_Renderer *renderer);
  void (*SDL_DestroyWindow)(SDL_Window *window);
  void (*SDL_RenderPresent)(SDL_Renderer *renderer);
  bool (*SDL_SetRenderLogicalPresentation)(SDL_Renderer *renderer, int w, int h, SDL_RendererLogicalPresentation mode);
  bool (*SDL_SetRenderDrawBlendMode)(SDL_Renderer *renderer,
                                     SDL_BlendMode blend_mode);
  bool (*SDL_SetRenderDrawColor)(SDL_Renderer *renderer, Uint8 r, Uint8 g,
                                 Uint8 b, Uint8 a);
  bool (*SDL_SetTextureBlendMode)(SDL_Texture *texture,
                                  SDL_BlendMode blend_mode);
  bool (*SDL_SetRenderScale)(SDL_Renderer *renderer, float scaleX, float scaleY);
  bool (*SDL_RenderClear)(SDL_Renderer *renderer);
  bool (*SDL_PollEvent)(SDL_Event *event);
  bool (*SDL_UpdateTexture)(SDL_Texture *texture, const SDL_Rect *rect,
                            const void *pixels, int pitch);
  bool (*SDL_RenderTexture)(SDL_Renderer *renderer, SDL_Texture *texture,
                            const SDL_FRect *srcrect, const SDL_FRect *dstrect);
  bool (*SDL_UpdateWindowSurface)(SDL_Window *window);
};

static void *try_find_sdl3_lib(void)
{
  char *candidates[] = {
      "/var/home/waffles/code/shiv/SDL/build/libSDL3.so",
      "/var/home/waffles/code/shiv/SDL/build/libSDL3.so.0",
      "./SDL/build/libSDL3.so",
      "./SDL/build/libSDL3.so.0",
      "libSDL3.so",
      "libSDL3.so.0",
      "libSDL3-0.so",
      "libSDL3.dylib",
      "SDL3.dll",
      "SDL3"};
  void *lib = NULL;
  for (size_t i = 0; i < (sizeof(candidates) / sizeof(*candidates)); ++i)
  {
    if ((lib = cosmo_dlopen(candidates[i], RTLD_LAZY)))
      return lib;
  }

  printf("Couldn't find SDL library (%s), tried the following names: ",
         cosmo_dlerror());
  for (size_t i = 0; i < (sizeof(candidates) / sizeof(*candidates)); ++i)
  {
    printf("\"%s\" ", candidates[i]);
  }
  printf("\n");

  return NULL;
}

static struct sdl_syms *try_get_sdl3_syms(void)
{
  void *sdl3 = try_find_sdl3_lib();

  if (!sdl3)
    return NULL;

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
      .SDL_SetRenderLogicalPresentation = cosmo_dlsym(sdl3, "SDL_SetRenderLogicalPresentation"),
      .SDL_SetRenderDrawBlendMode = cosmo_dlsym(sdl3, "SDL_SetRenderDrawBlendMode"),
      .SDL_SetRenderDrawColor = cosmo_dlsym(sdl3, "SDL_SetRenderDrawColor"),
      .SDL_SetTextureBlendMode = cosmo_dlsym(sdl3, "SDL_SetTextureBlendMode"),
      .SDL_SetRenderScale = cosmo_dlsym(sdl3, "SDL_SetRenderScale"),
      .SDL_RenderClear = cosmo_dlsym(sdl3, "SDL_RenderClear"),
      .SDL_PollEvent = cosmo_dlsym(sdl3, "SDL_PollEvent"),
      .SDL_UpdateTexture = cosmo_dlsym(sdl3, "SDL_UpdateTexture"),
      .SDL_RenderTexture = cosmo_dlsym(sdl3, "SDL_RenderTexture"),
      .SDL_UpdateWindowSurface = cosmo_dlsym(sdl3, "SDL_UpdateWindowSurface"),
  };

  return syms;
}

struct color
{
  float r, g, b, a;
};

// It's not obvious to me what p, q and t are supposed to be short for, copied
// here verbatim.
static float hue_to_rgb(float p, float q, float t)
{
  if (t < 0.0f)
    t += 1.0f;
  if (t > 1.0f)
    t -= 1.0f;
  if (t < 1.0f / 6.0f)
    return p + (q - p) * 6.0f * t;
  if (t < 1.0f / 2.0f)
    return q;
  if (t < 2.0f / 3.0f)
    return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
  return p;
}

static struct color color_from_hsl(float hue, float saturation,
                                   float lightness)
{
  // Map these to HSL standard ranges. 0-360 for h, 0-100 for s and l
  hue = hue / 360.0f;
  saturation = saturation / 100.0f;
  lightness = lightness / 100.0f;
  if (saturation == 0.0f)
  {
    return (struct color){lightness, lightness, lightness, 1.0f};
  }
  else
  {
    const float q = lightness < 0.5f
                        ? lightness * (1.0f + saturation)
                        : lightness + saturation - lightness * saturation;
    const float p = 2.0f * lightness - q;
    return (struct color){hue_to_rgb(p, q, hue + 1.0f / 3.0f),
                          hue_to_rgb(p, q, hue),
                          hue_to_rgb(p, q, hue - 1.0f / 3.0f), 1.0f};
  }
}

#define WIDTH 1280
#define HEIGHT 800

int main(void)
{
  printf("\n\n\n\n\nlets go!\n\n\n\n\n");
  struct sdl_syms *sym = try_get_sdl3_syms();

  if (!sym)
  {
    printf("Failed to get symbols, bailing out.\n");
    return -1;
  }

  if (!sym->SDL_Init(SDL_INIT_VIDEO))
  {
    printf("SDL couldn't initialize, error: \"%s\"\n", sym->SDL_GetError());
    return -1;
  }

  uint32_t flags = SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE;
  SDL_Window *window = sym->SDL_CreateWindow(
      "shiv © Christopher C Wahlfeldt, press Q or click on "
      "window to exit",
      WIDTH, HEIGHT, flags);

  if (!window)
  {
    printf("Window couldn't be created, error: \"%s\"\n", sym->SDL_GetError());
    return -1;
  }

  SDL_Renderer *renderer = sym->SDL_CreateRenderer(window, NULL);

  if (!renderer)
  {
    printf("Renderer couldn't be created, error: \"%s\"\n",
           sym->SDL_GetError());

    return -1;
  }

  sym->SDL_SetRenderLogicalPresentation(renderer, WIDTH, HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);
  sym->SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  sym->SDL_SetRenderScale(renderer, 1.0f, 1.0f);

  float hue = 0.0f;
  bool running = true;

  while (running)
  {
    SDL_Event event = {0};

    while (sym->SDL_PollEvent(&event))
    {
      if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0)
      {
        if (event.key.scancode == SDL_SCANCODE_Q)
        {
          running = false;
        }
      }
      if (event.type == SDL_EVENT_QUIT)
      {
        running = false;
      }
    }

    hue += 0.5f;
    if (hue >= 360.0f)
      hue = 0.0f;

    struct color c = color_from_hsl(hue, 100, 50);

    sym->SDL_SetRenderDrawColor(renderer, c.r * 255, c.g * 255, c.b * 255, 128);
    sym->SDL_RenderClear(renderer);
    sym->SDL_RenderPresent(renderer);
    sym->SDL_UpdateWindowSurface(window);
    sym->SDL_Delay(16);
  }

  sym->SDL_DestroyRenderer(renderer);
  sym->SDL_DestroyWindow(window);
  sym->SDL_Quit();

  cosmo_dlclose(sym->lib);
  free(sym);

  return 0;
}
