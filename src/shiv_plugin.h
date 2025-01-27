#ifndef SHIV_PLUGIN_H
#define SHIV_PLUGIN_H

#include <stdint.h>
#include <stdbool.h>
#include "shiv_sdl.h"

// Message types for communication
enum message_type {
    MSG_UPDATE = 1,  // Match with the value we're seeing in debug
    MSG_RENDER = 2,
    MSG_STATE = 3,
};

// Message structure for communication
struct plugin_message {
    enum message_type type;
    union {
        struct {
            float hue;
            bool running;
        } state;
        struct color color;
    } data;
};

#endif // SHIV_PLUGIN_H