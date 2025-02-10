#include <shiv/shiv.h>

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

// Matrix Math

Matrix4x4 Matrix4x4_multiply(Matrix4x4 matrix1, Matrix4x4 matrix2)
{
    Matrix4x4 result;

    result.m11 = ((matrix1.m11 * matrix2.m11) + (matrix1.m12 * matrix2.m21) + (matrix1.m13 * matrix2.m31) + (matrix1.m14 * matrix2.m41));
    result.m12 = ((matrix1.m11 * matrix2.m12) + (matrix1.m12 * matrix2.m22) + (matrix1.m13 * matrix2.m32) + (matrix1.m14 * matrix2.m42));
    result.m13 = ((matrix1.m11 * matrix2.m13) + (matrix1.m12 * matrix2.m23) + (matrix1.m13 * matrix2.m33) + (matrix1.m14 * matrix2.m43));
    result.m14 = ((matrix1.m11 * matrix2.m14) + (matrix1.m12 * matrix2.m24) + (matrix1.m13 * matrix2.m34) + (matrix1.m14 * matrix2.m44));
    result.m21 = ((matrix1.m21 * matrix2.m11) + (matrix1.m22 * matrix2.m21) + (matrix1.m23 * matrix2.m31) + (matrix1.m24 * matrix2.m41));
    result.m22 = ((matrix1.m21 * matrix2.m12) + (matrix1.m22 * matrix2.m22) + (matrix1.m23 * matrix2.m32) + (matrix1.m24 * matrix2.m42));
    result.m23 = ((matrix1.m21 * matrix2.m13) + (matrix1.m22 * matrix2.m23) + (matrix1.m23 * matrix2.m33) + (matrix1.m24 * matrix2.m43));
    result.m24 = ((matrix1.m21 * matrix2.m14) + (matrix1.m22 * matrix2.m24) + (matrix1.m23 * matrix2.m34) + (matrix1.m24 * matrix2.m44));
    result.m31 = ((matrix1.m31 * matrix2.m11) + (matrix1.m32 * matrix2.m21) + (matrix1.m33 * matrix2.m31) + (matrix1.m34 * matrix2.m41));
    result.m32 = ((matrix1.m31 * matrix2.m12) + (matrix1.m32 * matrix2.m22) + (matrix1.m33 * matrix2.m32) + (matrix1.m34 * matrix2.m42));
    result.m33 = ((matrix1.m31 * matrix2.m13) + (matrix1.m32 * matrix2.m23) + (matrix1.m33 * matrix2.m33) + (matrix1.m34 * matrix2.m43));
    result.m34 = ((matrix1.m31 * matrix2.m14) + (matrix1.m32 * matrix2.m24) + (matrix1.m33 * matrix2.m34) + (matrix1.m34 * matrix2.m44));
    result.m41 = ((matrix1.m41 * matrix2.m11) + (matrix1.m42 * matrix2.m21) + (matrix1.m43 * matrix2.m31) + (matrix1.m44 * matrix2.m41));
    result.m42 = ((matrix1.m41 * matrix2.m12) + (matrix1.m42 * matrix2.m22) + (matrix1.m43 * matrix2.m32) + (matrix1.m44 * matrix2.m42));
    result.m43 = ((matrix1.m41 * matrix2.m13) + (matrix1.m42 * matrix2.m23) + (matrix1.m43 * matrix2.m33) + (matrix1.m44 * matrix2.m43));
    result.m44 = ((matrix1.m41 * matrix2.m14) + (matrix1.m42 * matrix2.m24) + (matrix1.m43 * matrix2.m34) + (matrix1.m44 * matrix2.m44));

    return result;
}

Matrix4x4 Matrix4x4_create_rotation_z(float radians)
{
    return (Matrix4x4) { SDL_cosf(radians),
        SDL_sinf(radians),
        0,
        0,
        -SDL_sinf(radians),
        SDL_cosf(radians),
        0,
        0,
        0,
        0,
        1,
        0,
        0,
        0,
        0,
        1 };
}

Matrix4x4 Matrix4x4_create_translation(float x, float y, float z)
{
    return (Matrix4x4) { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, y, z, 1 };
}

Matrix4x4 Matrix4x4_create_orthographic_off_center(float left, float right, float bottom, float top, float zNearPlane,
    float zFarPlane)
{
    return (Matrix4x4) { 2.0f / (right - left),
        0,
        0,
        0,
        0,
        2.0f / (top - bottom),
        0,
        0,
        0,
        0,
        1.0f / (zNearPlane - zFarPlane),
        0,
        (left + right) / (left - right),
        (top + bottom) / (bottom - top),
        zNearPlane / (zNearPlane - zFarPlane),
        1 };
}

Matrix4x4 Matrix4x4_create_perspective_field_of_view(float fieldOfView, float aspectRatio, float nearPlaneDistance,
    float farPlaneDistance)
{
    float num = 1.0f / ((float)SDL_tanf(fieldOfView * 0.5f));
    return (Matrix4x4) { num / aspectRatio,
        0,
        0,
        0,
        0,
        num,
        0,
        0,
        0,
        0,
        farPlaneDistance / (nearPlaneDistance - farPlaneDistance),
        -1,
        0,
        0,
        (nearPlaneDistance * farPlaneDistance) / (nearPlaneDistance - farPlaneDistance),
        0 };
}

Matrix4x4 Matrix4x4_create_look_at(Vector3 cameraPosition, Vector3 cameraTarget, Vector3 cameraUpVector)
{
    Vector3 targetToPosition = { cameraPosition.x - cameraTarget.x, cameraPosition.y - cameraTarget.y,
        cameraPosition.z - cameraTarget.z };
    Vector3 vectorA = Vector3_normalize(targetToPosition);
    Vector3 vectorB = Vector3_normalize(Vector3_cross(cameraUpVector, vectorA));
    Vector3 vectorC = Vector3_cross(vectorA, vectorB);

    return (Matrix4x4) { vectorB.x,
        vectorC.x,
        vectorA.x,
        0,
        vectorB.y,
        vectorC.y,
        vectorA.y,
        0,
        vectorB.z,
        vectorC.z,
        vectorA.z,
        0,
        -Vector3_dot(vectorB, cameraPosition),
        -Vector3_dot(vectorC, cameraPosition),
        -Vector3_dot(vectorA, cameraPosition),
        1 };
}

Vector3 Vector3_normalize(Vector3 vec)
{
    float magnitude = SDL_sqrtf((vec.x * vec.x) + (vec.y * vec.y) + (vec.z * vec.z));
    return (Vector3) { vec.x / magnitude, vec.y / magnitude, vec.z / magnitude };
}

float Vector3_dot(Vector3 vecA, Vector3 vecB)
{
    return (vecA.x * vecB.x) + (vecA.y * vecB.y) + (vecA.z * vecB.z);
}

Vector3 Vector3_cross(Vector3 vecA, Vector3 vecB)
{
    return (Vector3) { vecA.y * vecB.z - vecB.y * vecA.z, -(vecA.x * vecB.z - vecB.x * vecA.z),
        vecA.x * vecB.y - vecB.x * vecA.y };
}
