#ifndef SDL_GPU_EXAMPLES_H
#define SDL_GPU_EXAMPLES_H

#include <SDL3/SDL.h>

typedef struct Renderer
{
    const char* name;
    const char* base_path;
    SDL_Window* window;
    SDL_GPUDevice* device;
    float delta_time;
} Renderer;

int renderer_init(Renderer* renderer, SDL_WindowFlags window_flags);
void renderer_quit(Renderer* renderer);

void initialize_asset_loader();
SDL_Surface* load_image(const char* image_filename, int desired_channels);
float* load_hdr_image(const char* image_filename, int* p_width, int* p_height, int* p_channels, int desired_channels);
void* load_astc_image(const char* image_filename, int* p_width, int* p_height, int* p_image_data_length);

SDL_GPUShader* load_shader(
    SDL_GPUDevice* device,
    const char* shader_filename,
    Uint32 sampler_count,
    Uint32 uniform_buffer_count,
    Uint32 storage_buffer_count,
    Uint32 storage_texture_count
);
SDL_GPUComputePipeline* create_compute_pipeline_from_shader(
    SDL_GPUDevice* device,
    const char* shader_filename,
    SDL_GPUComputePipelineCreateInfo* create_info
);

// Vertex Formats
typedef struct PositionVertex
{
    float x, y, z;
} PositionVertex;

typedef struct PositionColorVertex
{
    float x, y, z;
    Uint8 r, g, b, a;
} PositionColorVertex;

typedef struct PositionTextureVertex
{
    float x, y, z;
    float u, v;
} PositionTextureVertex;

// Matrix Math
typedef struct Matrix4x4
{
    float m11, m12, m13, m14;
    float m21, m22, m23, m24;
    float m31, m32, m33, m34;
    float m41, m42, m43, m44;
} Matrix4x4;

typedef struct Vector3
{
    float x, y, z;
} Vector3;

Matrix4x4 Matrix4x4_multiply(Matrix4x4 matrix1, Matrix4x4 matrix2);
Matrix4x4 Matrix4x4_create_rotation_z(float radians);
Matrix4x4 Matrix4x4_create_translation(float x, float y, float z);
Matrix4x4 Matrix4x4_create_orthographic_off_center(float left, float right, float bottom, float top, float z_near_plane, float z_far_plane);
Matrix4x4 Matrix4x4_create_perspective_field_of_view(float field_of_view, float aspect_ratio, float near_plane_distance, float far_plane_distance);
Matrix4x4 Matrix4x4_create_look_at(Vector3 camera_position, Vector3 camera_target, Vector3 camera_up_vector);
Vector3 Vector3_normalize(Vector3 vec);
float Vector3_dot(Vector3 vec_a, Vector3 vec_b);
Vector3 Vector3_cross(Vector3 vec_a, Vector3 vec_b);

// Examples
typedef struct System
{
    const char* name;
    int (*init)(Renderer* renderer);
    int (*update)(Renderer* renderer);
    int (*draw)(Renderer* renderer);
    void (*quit)(Renderer* renderer);
} System;

extern System depth_sampler;

#endif