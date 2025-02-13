#include <SDL3/SDL_gpu.h>
#include <shiv/renderer/renderer_utils.h>

QuadResources* create_fullscreen_quad(SDL_GPUDevice* device) {
    QuadResources* quad = SDL_malloc(sizeof(QuadResources));

    // Create vertex buffer
    quad->vertex_buffer = SDL_CreateGPUBuffer(
        device,
        &(SDL_GPUBufferCreateInfo) {
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(PositionTextureVertex) * 4
        });

    // Create index buffer
    quad->index_buffer = SDL_CreateGPUBuffer(
        device,
        &(SDL_GPUBufferCreateInfo) {
            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
            .size = sizeof(Uint16) * 6
        });

    // Create and map transfer buffer
    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(
        device,
        &(SDL_GPUTransferBufferCreateInfo) {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = sizeof(PositionTextureVertex) * 4 + sizeof(Uint16) * 6
        });

    PositionTextureVertex* transfer_data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);

    // Setup quad vertices
    PositionTextureVertex quad_vertices[] = {
        {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},   // Top-left
        {{1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},    // Top-right
        {{1.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},   // Bottom-right
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}}   // Bottom-left
    };
    SDL_memcpy(transfer_data, quad_vertices, sizeof(quad_vertices));

    // Setup indices
    Uint16* index_data = (Uint16*)&transfer_data[4];
    Uint16 indices[] = {0, 1, 2, 0, 2, 3};
    SDL_memcpy(index_data, indices, sizeof(indices));

    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

    // Upload data
    SDL_GPUCommandBuffer* cmd_buf = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd_buf);

    SDL_UploadToGPUBuffer(
        copy_pass,
        &(SDL_GPUTransferBufferLocation) {
            .transfer_buffer = transfer_buffer,
            .offset = 0
        },
        &(SDL_GPUBufferRegion) {
            .buffer = quad->vertex_buffer,
            .offset = 0,
            .size = sizeof(PositionTextureVertex) * 4
        },
        false);

    SDL_UploadToGPUBuffer(
        copy_pass,
        &(SDL_GPUTransferBufferLocation) {
            .transfer_buffer = transfer_buffer,
            .offset = sizeof(PositionTextureVertex) * 4
        },
        &(SDL_GPUBufferRegion) {
            .buffer = quad->index_buffer,
            .offset = 0,
            .size = sizeof(Uint16) * 6
        },
        false);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(cmd_buf);
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

    return quad;
}

void destroy_fullscreen_quad(SDL_GPUDevice* device, QuadResources* quad) {
    SDL_ReleaseGPUBuffer(device, quad->vertex_buffer);
    SDL_ReleaseGPUBuffer(device, quad->index_buffer);
    SDL_free(quad);
}

RenderTarget* create_render_target(SDL_GPUDevice* device, int width, int height,
                                 SDL_GPUTextureFormat color_format,
                                 SDL_GPUTextureFormat depth_format) {
    RenderTarget* target = SDL_malloc(sizeof(RenderTarget));
    target->width = width;
    target->height = height;

    target->color = SDL_CreateGPUTexture(
        device,
        &(SDL_GPUTextureCreateInfo) {
            .type = SDL_GPU_TEXTURETYPE_2D,
            .width = width,
            .height = height,
            .layer_count_or_depth = 1,
            .num_levels = 1,
            .sample_count = SDL_GPU_SAMPLECOUNT_1,
            .format = color_format,
            .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET
        });

    if (depth_format != SDL_GPU_TEXTUREFORMAT_INVALID) {
        target->depth = SDL_CreateGPUTexture(
            device,
            &(SDL_GPUTextureCreateInfo) {
                .type = SDL_GPU_TEXTURETYPE_2D,
                .width = width,
                .height = height,
                .layer_count_or_depth = 1,
                .num_levels = 1,
                .sample_count = SDL_GPU_SAMPLECOUNT_1,
                .format = depth_format,
                .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET
            });
    }

    return target;
}

void destroy_render_target(SDL_GPUDevice* device, RenderTarget* target) {
    SDL_ReleaseGPUTexture(device, target->color);
    if (target->depth) {
        SDL_ReleaseGPUTexture(device, target->depth);
    }
    SDL_free(target);
}

SDL_GPUColorTargetInfo create_color_target_info(
    SDL_GPUTexture* texture,
    SDL_FColor clear_color,
    SDL_GPULoadOp load_op,
    SDL_GPUStoreOp store_op) {

    return (SDL_GPUColorTargetInfo) {
        .texture = texture,
        .clear_color = clear_color,
        .load_op = load_op,
        .store_op = store_op
    };
}

SDL_GPUDepthStencilTargetInfo create_depth_target_info(
    SDL_GPUTexture* texture,
    float clear_depth,
    uint8_t clear_stencil,
    SDL_GPULoadOp load_op,
    SDL_GPUStoreOp store_op) {

    return (SDL_GPUDepthStencilTargetInfo) {
        .texture = texture,
        .clear_depth = clear_depth,
        .clear_stencil = clear_stencil,
        .load_op = load_op,
        .store_op = store_op,
        .stencil_load_op = load_op,
        .stencil_store_op = store_op
    };
}

SDL_GPUGraphicsPipelineCreateInfo create_default_3d_pipeline_info(
    SDL_GPUTextureFormat color_format,
    SDL_GPUTextureFormat depth_format,
    const SDL_GPUVertexInputState* vertex_input) {

    return (SDL_GPUGraphicsPipelineCreateInfo) {
        .target_info = {
            .num_color_targets = 1,
            .color_target_descriptions = (SDL_GPUColorTargetDescription[]) {
                { .format = color_format }
            },
            .has_depth_stencil_target = depth_format != SDL_GPU_TEXTUREFORMAT_INVALID,
            .depth_stencil_format = depth_format
        },
        .depth_stencil_state = (SDL_GPUDepthStencilState) {
            .enable_depth_test = true,
            .enable_depth_write = true,
            .enable_stencil_test = false,
            .compare_op = SDL_GPU_COMPAREOP_LESS,
            .write_mask = 0xFF
        },
        .rasterizer_state = (SDL_GPURasterizerState) {
            .cull_mode = SDL_GPU_CULLMODE_NONE,
            .fill_mode = SDL_GPU_FILLMODE_FILL,
            .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
        },
        .vertex_input_state = *vertex_input,
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST
    };
}

SDL_GPUGraphicsPipelineCreateInfo create_default_2d_pipeline_info(
    SDL_GPUTextureFormat color_format,
    const SDL_GPUVertexInputState* vertex_input,
    bool alpha_blend) {

    SDL_GPUGraphicsPipelineCreateInfo info = {
        .target_info = {
            .num_color_targets = 1,
            .color_target_descriptions = (SDL_GPUColorTargetDescription[]) {
                {
                    .format = color_format,
                    .blend_state = {
                        .enable_blend = alpha_blend,
                        .src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
                        .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                        .color_blend_op = SDL_GPU_BLENDOP_ADD,
                        .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
                        .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                        .alpha_blend_op = SDL_GPU_BLENDOP_ADD
                    }
                }
            }
        },
        .rasterizer_state = (SDL_GPURasterizerState) {
            .cull_mode = SDL_GPU_CULLMODE_NONE,
            .fill_mode = SDL_GPU_FILLMODE_FILL,
            .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
        },
        .vertex_input_state = *vertex_input,
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST
    };

    return info;
}
