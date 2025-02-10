#ifndef ENGINE_CORE_H
#define ENGINE_CORE_H

#include <SDL3/SDL.h>
#include <flecs.h>

#include "../renderer/renderer.h"

typedef struct Engine {
    const char* name;
    ecs_world_t* world;
    SDL_Window* window;
    Renderer* renderer;
    float delta_time;
} Engine;

#endif
