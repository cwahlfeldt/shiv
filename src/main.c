#define _COSMO_SOURCE
#include "libc/dlopen/dlfcn.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "shiv_color.h"
#include "shiv_plugin.h"
#include "shiv_sdl.h"

#define WIDTH 1280
#define HEIGHT 800

struct plugin_host {
  pid_t pid;
  int to_plugin[2];   // Pipe to send data to plugin
  int from_plugin[2]; // Pipe to receive data from plugin
  time_t last_modified;
};

static time_t get_file_mtime(const char *path) {
  struct stat st;
  if (stat(path, &st) == 0) {
    return st.st_mtime;
  }
  return 0;
}

static void cleanup_plugin(struct plugin_host *plugin) {
  if (plugin->pid > 0) {
    kill(plugin->pid, SIGTERM);
    waitpid(plugin->pid, NULL, 0);
    plugin->pid = 0;
  }

  close(plugin->to_plugin[0]);
  close(plugin->to_plugin[1]);
  close(plugin->from_plugin[0]);
  close(plugin->from_plugin[1]);
}

static bool launch_plugin(struct plugin_host *plugin, const char *plugin_path) {
  // Create pipes
  if (pipe(plugin->to_plugin) == -1 || pipe(plugin->from_plugin) == -1) {
    perror("Failed to create pipes");
    return false;
  }

  // Convert FDs to strings for args
  char read_fd[16], write_fd[16];
  snprintf(read_fd, sizeof(read_fd), "%d", plugin->to_plugin[0]);
  snprintf(write_fd, sizeof(write_fd), "%d", plugin->from_plugin[1]);

  // Prepare args
  char *const args[] = {(char *)plugin_path, read_fd, write_fd, NULL};

  // Launch plugin
  int status = posix_spawn(&plugin->pid, plugin_path, NULL, NULL, args, NULL);
  if (status != 0) {
    perror("Failed to spawn plugin");
    return false;
  }

  // Close unused ends of pipes
  close(plugin->to_plugin[0]);
  close(plugin->from_plugin[1]);

  // Set remaining pipe ends to non-blocking
  int flags;
  flags = fcntl(plugin->to_plugin[1], F_GETFL, 0);
  fcntl(plugin->to_plugin[1], F_SETFL, flags | O_NONBLOCK);
  flags = fcntl(plugin->from_plugin[0], F_GETFL, 0);
  fcntl(plugin->from_plugin[0], F_SETFL, flags | O_NONBLOCK);

  // Store initial modification time
  plugin->last_modified = get_file_mtime(plugin_path);
  return true;
}

static bool check_and_reload_plugin(struct plugin_host *plugin,
                                    const char *plugin_path) {
  time_t current_mtime = get_file_mtime(plugin_path);
  if (current_mtime <= plugin->last_modified) {
    return false;
  }

  printf("Hot reloading plugin...\n");
  cleanup_plugin(plugin);

  // Small delay to ensure file is fully written
  usleep(100000);

  if (!launch_plugin(plugin, plugin_path)) {
    printf("Failed to reload plugin\n");
    return false;
  }

  printf("Plugin reloaded successfully!\n");
  return true;
}

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

  printf("Couldn't find SDL library (%s), tried the following names: ",
         cosmo_dlerror());
  for (size_t i = 0; i < (sizeof(candidates) / sizeof(*candidates)); ++i) {
    printf("\"%s\" ", candidates[i]);
  }
  printf("\n");

  return NULL;
}

static struct sdl_syms *try_get_sdl3_syms(void) {
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
  printf("\n\n\n\n\nlets go!\n\n\n\n\n");
  struct sdl_syms *sym = try_get_sdl3_syms();

  if (!sym) {
    printf("Failed to get symbols, bailing out.\n");
    return -1;
  }

  if (!sym->SDL_Init(SDL_INIT_VIDEO)) {
    printf("SDL couldn't initialize, error: \"%s\"\n", sym->SDL_GetError());
    return -1;
  }

  // Create window and renderer
  uint32_t flags = SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE;
  SDL_Window *window = sym->SDL_CreateWindow(
      "shiv © Christopher C Wahlfeldt, press Q or click on window to exit",
      WIDTH, HEIGHT, flags);

  if (!window) {
    printf("Window couldn't be created, error: \"%s\"\n", sym->SDL_GetError());
    return -1;
  }

  SDL_Renderer *renderer = sym->SDL_CreateRenderer(window, NULL);
  if (!renderer) {
    printf("Renderer couldn't be created, error: \"%s\"\n",
           sym->SDL_GetError());
    sym->SDL_DestroyWindow(window);
    return -1;
  }

  sym->SDL_SetRenderLogicalPresentation(renderer, WIDTH, HEIGHT,
                                        SDL_LOGICAL_PRESENTATION_LETTERBOX);
  sym->SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  sym->SDL_SetRenderScale(renderer, 1.0f, 1.0f);

  // Initialize plugin system
  struct plugin_host plugin = {0};
  if (!launch_plugin(&plugin, "build/plugin")) {
    printf("Failed to launch plugin\n");
    sym->SDL_DestroyRenderer(renderer);
    sym->SDL_DestroyWindow(window);
    sym->SDL_Quit();
    return -1;
  }

  struct plugin_message msg = {.type = MSG_UPDATE,
                               .data.state = {.hue = 0.0f, .running = true}};

  // Main game loop
  while (msg.data.state.running) {
    SDL_Event event = {0};
    while (sym->SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0) {
        if (event.key.scancode == SDL_SCANCODE_Q) {
          msg.data.state.running = false;
        }
      }
      if (event.type == SDL_EVENT_QUIT) {
        msg.data.state.running = false;
      }
    }

    // Check for plugin changes
    check_and_reload_plugin(&plugin, "build/plugin");

    static uint32_t last_update = 0;
    uint32_t current_time = sym->SDL_GetTicks();

    // Send update message every 16ms (roughly 60fps)
    if (current_time - last_update >= 16) {
      msg.type = MSG_UPDATE; // Ensure we're sending update message

      if (write(plugin.to_plugin[1], &msg, sizeof(msg)) < 0 &&
          errno != EAGAIN) {
        printf("Failed to write to plugin: %d\n", errno);
        break;
      }
      printf("Main: sent update message\n");
      last_update = current_time;
    }

    // Handle plugin messages
    int bytes;
    while ((bytes = read(plugin.from_plugin[0], &msg, sizeof(msg))) > 0 ||
           (bytes < 0 && errno == EAGAIN)) {
      if (bytes < 0)
        continue; // Skip EAGAIN case

      printf("Main: received message type: %d\n", msg.type);
      switch (msg.type) {
      case MSG_STATE:
        printf("Main: state update - hue: %f\n", msg.data.state.hue);
        break;

      case MSG_RENDER: {
        // Handle render command - use color directly from message
        printf("Main: rendering color r=%f g=%f b=%f\n", msg.data.color.r,
               msg.data.color.g, msg.data.color.b);
        sym->SDL_SetRenderDrawColor(renderer, (uint8_t)(msg.data.color.r * 255),
                                    (uint8_t)(msg.data.color.g * 255),
                                    (uint8_t)(msg.data.color.b * 255), 128);
        sym->SDL_RenderClear(renderer);
        sym->SDL_RenderPresent(renderer);
        break;
      }
      }
    }

    // Short sleep to prevent busy-waiting
    usleep(1000);

    sym->SDL_Delay(16);
  }

  // Cleanup
  cleanup_plugin(&plugin);
  sym->SDL_DestroyRenderer(renderer);
  sym->SDL_DestroyWindow(window);
  sym->SDL_Quit();
  free(sym);

  return 0;
}
