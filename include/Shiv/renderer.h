/**
 * @file renderer.h
 * @brief Core renderer, built on SDL3_gpu
 */

#ifndef RENDERER_H
#define RENDERER_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Renderer {
    SDL_Window* window;
    SDL_GPUDevice* gpu_device;
    SDL_GPUTexture* swapchain_texture;
    SDL_GPUCommandBuffer* cmd_buffer;
    bool running;
} Renderer;

bool init(Renderer* renderer);
void cleanup(Renderer* renderer);
void handle_events(Renderer* renderer);
bool render_frame(Renderer* renderer);
int render();

#ifdef __cplusplus
}
#endif

#endif
