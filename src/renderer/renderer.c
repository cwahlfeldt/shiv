#include <shiv/shiv.h>
#include <cglm/cglm.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_MALLOC SDL_malloc
#define STBI_REALLOC SDL_realloc
#define STBI_FREE SDL_free
#define STBI_ONLY_HDR
#include "../../lib/stb/stb_image.h"

int renderer_init(Renderer* renderer, SDL_WindowFlags window_flags)
{
    renderer->device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, false, NULL);

    if (renderer->device == NULL) {
        SDL_Log("GPUCreateDevice failed");
        return -1;
    }

    renderer->window = SDL_CreateWindow(renderer->name, 1024, 760, window_flags);
    if (renderer->window == NULL) {
        SDL_Log("CreateWindow failed: %s", SDL_GetError());
        return -1;
    }

    if (!SDL_ClaimWindowForGPUDevice(renderer->device, renderer->window)) {
        SDL_Log("GPUClaimWindow failed");
        return -1;
    }

    return 0;
}

void renderer_quit(Renderer* renderer)
{
    SDL_ReleaseWindowFromGPUDevice(renderer->device, renderer->window);
    SDL_DestroyWindow(renderer->window);
    SDL_DestroyGPUDevice(renderer->device);
}

static const char* base_path = NULL;
void initialize_asset_loader()
{
    base_path = SDL_GetBasePath();
}

SDL_GPUShader* load_shader(SDL_GPUDevice* device, const char* shader_filename, Uint32 sampler_count,
    Uint32 uniform_buffer_count, Uint32 storage_buffer_count, Uint32 storage_texture_count)
{
    // Auto-detect the shader stage from the file name for convenience
    SDL_GPUShaderStage stage;
    if (SDL_strstr(shader_filename, ".vert")) {
        stage = SDL_GPU_SHADERSTAGE_VERTEX;
    } else if (SDL_strstr(shader_filename, ".frag")) {
        stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    } else {
        SDL_Log("Invalid shader stage!");
        return NULL;
    }

    char full_path[256];
    SDL_GPUShaderFormat backend_formats = SDL_GetGPUShaderFormats(device);
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
    const char* entrypoint;

    if (backend_formats & SDL_GPU_SHADERFORMAT_SPIRV) {
        SDL_snprintf(full_path, sizeof(full_path), "%sshaders/compiled/SPIRV/%s.spv", base_path, shader_filename);
        format = SDL_GPU_SHADERFORMAT_SPIRV;
        entrypoint = "main";
    } else if (backend_formats & SDL_GPU_SHADERFORMAT_MSL) {
        SDL_snprintf(full_path, sizeof(full_path), "%sshaders/compiled/MSL/%s.msl", base_path, shader_filename);
        format = SDL_GPU_SHADERFORMAT_MSL;
        entrypoint = "main0";
    } else if (backend_formats & SDL_GPU_SHADERFORMAT_DXIL) {
        SDL_snprintf(full_path, sizeof(full_path), "%sshaders/compiled/DXIL/%s.dxil", base_path, shader_filename);
        format = SDL_GPU_SHADERFORMAT_DXIL;
        entrypoint = "main";
    } else {
        SDL_Log("%s", "Unrecognized backend shader format!");
        return NULL;
    }

    size_t code_size;
    void* code = SDL_LoadFile(full_path, &code_size);
    if (code == NULL) {
        SDL_Log("Failed to load shader from disk! %s", full_path);
        return NULL;
    }

    SDL_GPUShaderCreateInfo shader_info = { .code = code,
        .code_size = code_size,
        .entrypoint = entrypoint,
        .format = format,
        .stage = stage,
        .num_samplers = sampler_count,
        .num_uniform_buffers = uniform_buffer_count,
        .num_storage_buffers = storage_buffer_count,
        .num_storage_textures = storage_texture_count };

    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shader_info);
    if (shader == NULL) {
        SDL_Log("Failed to create shader!");
        SDL_free(code);
        return NULL;
    }

    SDL_free(code);
    return shader;
}

SDL_GPUComputePipeline* create_compute_pipeline_from_shader(SDL_GPUDevice* device, const char* shader_filename,
    SDL_GPUComputePipelineCreateInfo* create_info)
{
    char full_path[256];
    SDL_GPUShaderFormat backend_formats = SDL_GetGPUShaderFormats(device);
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
    const char* entrypoint;

    if (backend_formats & SDL_GPU_SHADERFORMAT_SPIRV) {
        SDL_snprintf(full_path, sizeof(full_path), "%sshaders/compiled/SPIRV/%s.spv", base_path, shader_filename);
        format = SDL_GPU_SHADERFORMAT_SPIRV;
        entrypoint = "main";
    } else if (backend_formats & SDL_GPU_SHADERFORMAT_MSL) {
        SDL_snprintf(full_path, sizeof(full_path), "%sshaders/compiled/MSL/%s.msl", base_path, shader_filename);
        format = SDL_GPU_SHADERFORMAT_MSL;
        entrypoint = "main0";
    } else if (backend_formats & SDL_GPU_SHADERFORMAT_DXIL) {
        SDL_snprintf(full_path, sizeof(full_path), "%sshaders/compiled/DXIL/%s.dxil", base_path, shader_filename);
        format = SDL_GPU_SHADERFORMAT_DXIL;
        entrypoint = "main";
    } else {
        SDL_Log("%s", "Unrecognized backend shader format!");
        return NULL;
    }

    size_t code_size;
    void* code = SDL_LoadFile(full_path, &code_size);
    if (code == NULL) {
        SDL_Log("Failed to load compute shader from disk! %s", full_path);
        return NULL;
    }

    // Make a copy of the create data, then overwrite the parts we need
    SDL_GPUComputePipelineCreateInfo new_create_info = *create_info;
    new_create_info.code = code;
    new_create_info.code_size = code_size;
    new_create_info.entrypoint = entrypoint;
    new_create_info.format = format;

    SDL_GPUComputePipeline* pipeline = SDL_CreateGPUComputePipeline(device, &new_create_info);
    if (pipeline == NULL) {
        SDL_Log("Failed to create compute pipeline!");
        SDL_free(code);
        return NULL;
    }

    SDL_free(code);
    return pipeline;
}

SDL_Surface* load_image(const char* image_filename, int desired_channels)
{
    char full_path[256];
    SDL_Surface* result;
    SDL_PixelFormat format;

    SDL_snprintf(full_path, sizeof(full_path), "%sassets/Images/%s", base_path, image_filename);

    result = SDL_LoadBMP(full_path);
    if (result == NULL) {
        SDL_Log("Failed to load BMP: %s", SDL_GetError());
        return NULL;
    }

    if (desired_channels == 4) {
        format = SDL_PIXELFORMAT_ABGR8888;
    } else {
        SDL_assert(!"Unexpected desired_channels");
        SDL_DestroySurface(result);
        return NULL;
    }
    if (result->format != format) {
        SDL_Surface* next = SDL_ConvertSurface(result, format);
        SDL_DestroySurface(result);
        result = next;
    }

    return result;
}

float* load_hdr_image(const char* image_filename, int* p_width, int* p_height, int* p_channels, int desired_channels)
{
    char full_path[256];
    SDL_snprintf(full_path, sizeof(full_path), "%sContent/Images/%s", base_path, image_filename);
    return stbi_loadf(full_path, p_width, p_height, p_channels, desired_channels);
}

typedef struct AstcHeader {
    Uint8 magic[4];
    Uint8 block_x;
    Uint8 block_y;
    Uint8 block_z;
    Uint8 dim_x[3];
    Uint8 dim_y[3];
    Uint8 dim_z[3];
} AstcHeader;

void* load_astc_image(const char* image_filename, int* p_width, int* p_height, int* p_image_data_length)
{
    char full_path[256];
    SDL_snprintf(full_path, sizeof(full_path), "%sContent/Images/astc/%s", base_path, image_filename);

    size_t file_size;
    void* file_contents = SDL_LoadFile(full_path, &file_size);
    if (file_contents == NULL) {
        SDL_assert(!"Could not load ASTC image!");
        return NULL;
    }

    AstcHeader* header = (AstcHeader*)file_contents;
    if (header->magic[0] != 0x13 || header->magic[1] != 0xAB || header->magic[2] != 0xA1 || header->magic[3] != 0x5C) {
        SDL_assert(!"Bad magic number!");
        return NULL;
    }

    // Get the image dimensions in texels
    *p_width = header->dim_x[0] + (header->dim_x[1] << 8) + (header->dim_x[2] << 16);
    *p_height = header->dim_y[0] + (header->dim_y[1] << 8) + (header->dim_y[2] << 16);

    // Get the size of the texture data
    unsigned int block_count_x = (*p_width + header->block_x - 1) / header->block_x;
    unsigned int block_count_y = (*p_height + header->block_y - 1) / header->block_y;
    *p_image_data_length = block_count_x * block_count_y * 16;

    void* data = SDL_malloc(*p_image_data_length);
    SDL_memcpy(data, (char*)file_contents + sizeof(AstcHeader), *p_image_data_length);
    SDL_free(file_contents);

    return data;
}
