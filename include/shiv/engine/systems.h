#ifndef ENGINE_SYSTEMS_H
#define ENGINE_SYSTEMS_H

#include "../renderer/renderer.h"

// Examples
typedef struct System {
    const char* name;
    int (*init)(Renderer* renderer);
    int (*update)(Renderer* renderer);
    int (*draw)(Renderer* renderer);
    void (*quit)(Renderer* renderer);
} System;

extern System depth_sampler;

#endif
