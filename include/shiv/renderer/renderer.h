#ifndef SDL_GPU_EXAMPLES_H
#define SDL_GPU_EXAMPLES_H

#include <SDL3/SDL.h>

typedef struct Renderer {
    const char* name;
    const char* base_path;
    SDL_Window* window;
    SDL_GPUDevice* device;
    float delta_time;
} Renderer;

int renderer_init(
    Renderer* renderer,
    SDL_WindowFlags window_flags);

void renderer_quit(Renderer* renderer);

void initialize_asset_loader();

SDL_Surface* load_image(
    const char* image_filename,
    int desired_channels);

float* load_hdr_image(
    const char* image_filename,
    int* p_width,
    int* p_height,
    int* p_channels,
    int desired_channels);

void* load_astc_image(
    const char* image_filename,
    int* p_width,
    int* p_height,
    int* p_image_data_length);

SDL_GPUShader* load_shader(
    SDL_GPUDevice* device,
    const char* shader_filename,
    Uint32 sampler_count,
    Uint32 uniform_buffer_count,
    Uint32 storage_buffer_count,
    Uint32 storage_texture_count);

SDL_GPUComputePipeline* create_compute_pipeline_from_shader(
    SDL_GPUDevice* device,
    const char* shader_filename,
    SDL_GPUComputePipelineCreateInfo* create_info);

#include <cglm/cglm.h>

// Vertex Formats with aligned data for GPU use
typedef struct {
    float position[3];  // Using array form for compatibility
} PositionVertex;

typedef struct {
    float position[3];  // Using array form for compatibility
    uint8_t r, g, b, a;
} PositionColorVertex;

typedef struct {
    float position[3];  // Using array form for compatibility
    float texcoord[2];  // Using array form for compatibility
} PositionTextureVertex;


#endif
