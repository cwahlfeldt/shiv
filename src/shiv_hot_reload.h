#ifndef SHIV_HOT_RELOAD_H
#define SHIV_HOT_RELOAD_H

#include <libc/calls/weirdtypes.h>
#include <stdbool.h>
#include <stdint.h>

// Generic hot reload interface
struct hot_reload_handler {
  void *data;                  // Handler-specific data
  bool (*init)(void *data);    // Initialize handler
  bool (*check)(void *data);   // Check for changes
  bool (*reload)(void *data);  // Perform reload
  void (*cleanup)(void *data); // Cleanup resources
};

// Plugin-specific hot reload data
struct plugin_reload_data {
  const char *plugin_path;
  void *plugin_handle;
  uint64_t last_modified;
  int pipes[2][2]; // [0] = to_plugin, [1] = from_plugin
  pid_t pid;
};

#ifdef SHIV_HOT_RELOAD
#define HOT_RELOAD_ENABLED true
bool hot_reload_init(struct hot_reload_handler *handler);
bool hot_reload_check(struct hot_reload_handler *handler);
void hot_reload_cleanup(struct hot_reload_handler *handler);
#else
#define HOT_RELOAD_ENABLED false
#define hot_reload_init(h) (true)
#define hot_reload_check(h) (true)
#define hot_reload_cleanup(h)
#endif

#endif // SHIV_HOT_RELOAD_H
