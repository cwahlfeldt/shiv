#ifndef RENDERER_UTILS_H
#define RENDERER_UTILS_H

#include <SDL3/SDL.h>
#include "renderer.h"

// Common resource structures
typedef struct QuadResources {
    SDL_GPUBuffer* vertex_buffer;
    SDL_GPUBuffer* index_buffer;
} QuadResources;

typedef struct RenderTarget {
    SDL_GPUTexture* color;
    SDL_GPUTexture* depth;
    int width;
    int height;
} RenderTarget;

// Creation/destruction functions
QuadResources* create_fullscreen_quad(SDL_GPUDevice* device);
void destroy_fullscreen_quad(SDL_GPUDevice* device, QuadResources* quad);

RenderTarget* create_render_target(SDL_GPUDevice* device, int width, int height,
                                 SDL_GPUTextureFormat color_format,
                                 SDL_GPUTextureFormat depth_format);

void destroy_render_target(SDL_GPUDevice* device, RenderTarget* target);

// Common render state helpers
SDL_GPUColorTargetInfo create_color_target_info(
    SDL_GPUTexture* texture,
    SDL_FColor clear_color,
    SDL_GPULoadOp load_op,
    SDL_GPUStoreOp store_op);

SDL_GPUDepthStencilTargetInfo create_depth_target_info(
    SDL_GPUTexture* texture,
    float clear_depth,
    uint8_t clear_stencil,
    SDL_GPULoadOp load_op,
    SDL_GPUStoreOp store_op);

// Pipeline creation helpers
SDL_GPUGraphicsPipelineCreateInfo create_default_3d_pipeline_info(
    SDL_GPUTextureFormat color_format,
    SDL_GPUTextureFormat depth_format,
    const SDL_GPUVertexInputState* vertex_input);

SDL_GPUGraphicsPipelineCreateInfo create_default_2d_pipeline_info(
    SDL_GPUTextureFormat color_format,
    const SDL_GPUVertexInputState* vertex_input,
    bool alpha_blend);

#endif // RENDERER_UTILS_H
