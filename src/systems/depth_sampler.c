#include <shiv/shiv.h>
#include <shiv/renderer/renderer_utils.h>

// Scene rendering resources
static SDL_GPUGraphicsPipeline* ScenePipeline;
static SDL_GPUBuffer* SceneVertexBuffer;
static SDL_GPUBuffer* SceneIndexBuffer;
static RenderTarget* SceneTarget;

// Post-processing resources
static SDL_GPUGraphicsPipeline* EffectPipeline;
static QuadResources* EffectQuad;
static SDL_GPUSampler* EffectSampler;

static float Time;

static int init(Renderer* renderer)
{
    // Basic initialization
    int result = renderer_init(renderer, 0);
    if (result < 0) {
        return result;
    }

    // Create render target at quarter resolution
    int w, h;
    SDL_GetWindowSizeInPixels(renderer->window, &w, &h);
    SceneTarget = create_render_target(renderer->device, w/4, h/4,
                                     SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                                     SDL_GPU_TEXTUREFORMAT_D16_UNORM);
    if (!SceneTarget) {
        SDL_Log("Failed to create scene render target!");
        return -1;
    }

    // Create the shaders
    SDL_GPUShader* scene_vertex_shader = load_shader(renderer->device, "PositionColorTransform.vert", 0, 1, 0, 0);
    SDL_GPUShader* scene_fragment_shader = load_shader(renderer->device, "SolidColorDepth.frag", 0, 1, 0, 0);
    SDL_GPUShader* effect_vertex_shader = load_shader(renderer->device, "TexturedQuad.vert", 0, 0, 0, 0);
    SDL_GPUShader* effect_fragment_shader = load_shader(renderer->device, "DepthOutline.frag", 2, 1, 0, 0);

    if (!scene_vertex_shader || !scene_fragment_shader || !effect_vertex_shader || !effect_fragment_shader) {
        SDL_Log("Failed to load shaders!");
        return -1;
    }

    // Create scene pipeline
    SDL_GPUVertexInputState scene_vertex_input = {
        .num_vertex_buffers = 1,
        .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]) {
            { .slot = 0, .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
              .instance_step_rate = 0, .pitch = sizeof(PositionColorVertex) }
        },
        .num_vertex_attributes = 2,
        .vertex_attributes = (SDL_GPUVertexAttribute[]) {
            { .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
              .location = 0, .offset = 0 },
            { .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
              .location = 1, .offset = sizeof(float) * 3 }
        }
    };

    SDL_GPUGraphicsPipelineCreateInfo scene_pipeline_info = 
        create_default_3d_pipeline_info(SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                                      SDL_GPU_TEXTUREFORMAT_D16_UNORM,
                                      &scene_vertex_input);
    scene_pipeline_info.vertex_shader = scene_vertex_shader;
    scene_pipeline_info.fragment_shader = scene_fragment_shader;

    ScenePipeline = SDL_CreateGPUGraphicsPipeline(renderer->device, &scene_pipeline_info);
    if (!ScenePipeline) {
        SDL_Log("Failed to create Scene pipeline!");
        return -1;
    }

    // Create effect pipeline
    SDL_GPUVertexInputState effect_vertex_input = {
        .num_vertex_buffers = 1,
        .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]) {
            { .slot = 0, .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
              .instance_step_rate = 0, .pitch = sizeof(PositionTextureVertex) }
        },
        .num_vertex_attributes = 2,
        .vertex_attributes = (SDL_GPUVertexAttribute[]) {
            { .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
              .location = 0, .offset = 0 },
            { .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
              .location = 1, .offset = sizeof(float) * 3 }
        }
    };

    SDL_GPUGraphicsPipelineCreateInfo effect_pipeline_info = 
        create_default_2d_pipeline_info(SDL_GetGPUSwapchainTextureFormat(renderer->device, renderer->window),
                                      &effect_vertex_input,
                                      true);  // Enable alpha blending
    effect_pipeline_info.vertex_shader = effect_vertex_shader;
    effect_pipeline_info.fragment_shader = effect_fragment_shader;

    EffectPipeline = SDL_CreateGPUGraphicsPipeline(renderer->device, &effect_pipeline_info);
    if (!EffectPipeline) {
        SDL_Log("Failed to create Effect pipeline!");
        return -1;
    }

    // Cleanup shaders
    SDL_ReleaseGPUShader(renderer->device, effect_vertex_shader);
    SDL_ReleaseGPUShader(renderer->device, effect_fragment_shader);
    SDL_ReleaseGPUShader(renderer->device, scene_vertex_shader);
    SDL_ReleaseGPUShader(renderer->device, scene_fragment_shader);

    // Create effect sampler
    EffectSampler = SDL_CreateGPUSampler(renderer->device, &(SDL_GPUSamplerCreateInfo) {
        .min_filter = SDL_GPU_FILTER_NEAREST,
        .mag_filter = SDL_GPU_FILTER_NEAREST,
        .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
        .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
        .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
    });

    // Create fullscreen quad for effect
    EffectQuad = create_fullscreen_quad(renderer->device);
    if (!EffectQuad) {
        SDL_Log("Failed to create effect quad!");
        return -1;
    }

    // Create scene buffers
    SceneVertexBuffer = SDL_CreateGPUBuffer(
        renderer->device,
        &(SDL_GPUBufferCreateInfo) {
            .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
            .size = sizeof(PositionColorVertex) * 24  // Cube vertices
        });

    SceneIndexBuffer = SDL_CreateGPUBuffer(
        renderer->device,
        &(SDL_GPUBufferCreateInfo) {
            .usage = SDL_GPU_BUFFERUSAGE_INDEX,
            .size = sizeof(Uint16) * 36  // Cube indices
        });

    // Upload scene geometry
    SDL_GPUTransferBuffer* buffer_transfer = SDL_CreateGPUTransferBuffer(
        renderer->device,
        &(SDL_GPUTransferBufferCreateInfo) {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = (sizeof(PositionColorVertex) * 24) + (sizeof(Uint16) * 36)
        });

    PositionColorVertex* transfer_data = SDL_MapGPUTransferBuffer(renderer->device, buffer_transfer, false);

    // Define cube vertices with colors
    PositionColorVertex cube_vertices[] = {
        {{-10.0f, -10.0f, -10.0f}, 255, 0, 0, 255},    // Front face (red)
        {{10.0f, -10.0f, -10.0f}, 255, 0, 0, 255},
        {{10.0f, 10.0f, -10.0f}, 255, 0, 0, 255},
        {{-10.0f, 10.0f, -10.0f}, 255, 0, 0, 255},
        
        {{-10.0f, -10.0f, 10.0f}, 255, 255, 0, 255},   // Back face (yellow)
        {{10.0f, -10.0f, 10.0f}, 255, 255, 0, 255},
        {{10.0f, 10.0f, 10.0f}, 255, 255, 0, 255},
        {{-10.0f, 10.0f, 10.0f}, 255, 255, 0, 255},
        
        {{-10.0f, -10.0f, -10.0f}, 255, 0, 255, 255},  // Left face (magenta)
        {{-10.0f, 10.0f, -10.0f}, 255, 0, 255, 255},
        {{-10.0f, 10.0f, 10.0f}, 255, 0, 255, 255},
        {{-10.0f, -10.0f, 10.0f}, 255, 0, 255, 255},
        
        {{10.0f, -10.0f, -10.0f}, 0, 255, 0, 255},     // Right face (green)
        {{10.0f, 10.0f, -10.0f}, 0, 255, 0, 255},
        {{10.0f, 10.0f, 10.0f}, 0, 255, 0, 255},
        {{10.0f, -10.0f, 10.0f}, 0, 255, 0, 255},
        
        {{-10.0f, -10.0f, -10.0f}, 0, 255, 255, 255},  // Bottom face (cyan)
        {{-10.0f, -10.0f, 10.0f}, 0, 255, 255, 255},
        {{10.0f, -10.0f, 10.0f}, 0, 255, 255, 255},
        {{10.0f, -10.0f, -10.0f}, 0, 255, 255, 255},
        
        {{-10.0f, 10.0f, -10.0f}, 0, 0, 255, 255},     // Top face (blue)
        {{-10.0f, 10.0f, 10.0f}, 0, 0, 255, 255},
        {{10.0f, 10.0f, 10.0f}, 0, 0, 255, 255},
        {{10.0f, 10.0f, -10.0f}, 0, 0, 255, 255}
    };
    memcpy(transfer_data, cube_vertices, sizeof(cube_vertices));

    // Define cube indices
    Uint16* index_data = (Uint16*)&transfer_data[24];
    Uint16 indices[] = {
        0, 1, 2, 0, 2, 3,       // Front
        4, 5, 6, 4, 6, 7,       // Back
        8, 9, 10, 8, 10, 11,    // Left
        12, 13, 14, 12, 14, 15, // Right
        16, 17, 18, 16, 18, 19, // Bottom
        20, 21, 22, 20, 22, 23  // Top
    };
    SDL_memcpy(index_data, indices, sizeof(indices));

    SDL_UnmapGPUTransferBuffer(renderer->device, buffer_transfer);

    // Upload vertex and index data
    SDL_GPUCommandBuffer* cmd_buf = SDL_AcquireGPUCommandBuffer(renderer->device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd_buf);

    SDL_UploadToGPUBuffer(
        copy_pass,
        &(SDL_GPUTransferBufferLocation) {
            .transfer_buffer = buffer_transfer,
            .offset = 0 
        },
        &(SDL_GPUBufferRegion) {
            .buffer = SceneVertexBuffer,
            .offset = 0,
            .size = sizeof(PositionColorVertex) * 24 
        },
        false);

    SDL_UploadToGPUBuffer(
        copy_pass,
        &(SDL_GPUTransferBufferLocation) {
            .transfer_buffer = buffer_transfer,
            .offset = sizeof(PositionColorVertex) * 24 
        },
        &(SDL_GPUBufferRegion) {
            .buffer = SceneIndexBuffer,
            .offset = 0,
            .size = sizeof(Uint16) * 36 
        },
        false);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(cmd_buf);
    SDL_ReleaseGPUTransferBuffer(renderer->device, buffer_transfer);

    Time = 0;
    return 0;
}

static int update(Renderer* renderer)
{
    Time += renderer->delta_time;
    return 0;
}

static int draw(Renderer* renderer)
{
    SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(renderer->device);
    if (!cmdbuf) {
        SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        return -1;
    }

    SDL_GPUTexture* swapchain_texture;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, renderer->window, &swapchain_texture, NULL, NULL)) {
        SDL_Log("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
        return -1;
    }

    if (swapchain_texture) {
        // Render the 3D Scene (Color and Depth pass)
        float near_plane = 20.0f;
        float far_plane = 60.0f;

        // Camera setup
        vec3 eye = { SDL_cosf(Time) * 30, 30, SDL_sinf(Time) * 30 };
        vec3 center = { 0, 0, 0 };
        vec3 up = { 0, 1, 0 };

        // Create view-projection matrix
        mat4 proj = GLM_MAT4_IDENTITY_INIT;
        mat4 view = GLM_MAT4_IDENTITY_INIT;
        mat4 viewproj = GLM_MAT4_IDENTITY_INIT;
        
        glm_perspective(75.0f * SDL_PI_F / 180.0f,
                       SceneTarget->width / (float)SceneTarget->height,
                       near_plane,
                       far_plane,
                       proj);
        glm_lookat(eye, center, up, view);
        glm_mat4_mul(proj, view, viewproj);

        // Scene pass
        SDL_GPUColorTargetInfo color_target = create_color_target_info(
            SceneTarget->color,
            (SDL_FColor) { 0.0f, 0.0f, 0.0f, 0.0f },
            SDL_GPU_LOADOP_CLEAR,
            SDL_GPU_STOREOP_STORE);

        SDL_GPUDepthStencilTargetInfo depth_target = create_depth_target_info(
            SceneTarget->depth,
            1.0f,
            0,
            SDL_GPU_LOADOP_CLEAR,
            SDL_GPU_STOREOP_STORE);
        depth_target.cycle = true;

        // Push uniforms and draw scene
        SDL_PushGPUVertexUniformData(cmdbuf, 0, viewproj, sizeof(viewproj));
        SDL_PushGPUFragmentUniformData(cmdbuf, 0, (float[]) { near_plane, far_plane }, 8);

        SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(cmdbuf, &color_target, 1, &depth_target);
        SDL_BindGPUVertexBuffers(render_pass, 0, &(SDL_GPUBufferBinding) { .buffer = SceneVertexBuffer, .offset = 0 }, 1);
        SDL_BindGPUIndexBuffer(render_pass, &(SDL_GPUBufferBinding) { .buffer = SceneIndexBuffer, .offset = 0 }, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        SDL_BindGPUGraphicsPipeline(render_pass, ScenePipeline);
        SDL_DrawGPUIndexedPrimitives(render_pass, 36, 1, 0, 0, 0);
        SDL_EndGPURenderPass(render_pass);

        // Post-process pass
        SDL_GPUColorTargetInfo swapchain_target = create_color_target_info(
            swapchain_texture,
            (SDL_FColor) { 0.2f, 0.5f, 0.4f, 1.0f },
            SDL_GPU_LOADOP_CLEAR,
            SDL_GPU_STOREOP_STORE);

        render_pass = SDL_BeginGPURenderPass(cmdbuf, &swapchain_target, 1, NULL);
        SDL_BindGPUGraphicsPipeline(render_pass, EffectPipeline);
        SDL_BindGPUVertexBuffers(render_pass, 0, &(SDL_GPUBufferBinding) { .buffer = EffectQuad->vertex_buffer, .offset = 0 }, 1);
        SDL_BindGPUIndexBuffer(render_pass, &(SDL_GPUBufferBinding) { .buffer = EffectQuad->index_buffer, .offset = 0 }, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        SDL_BindGPUFragmentSamplers(render_pass, 0, (SDL_GPUTextureSamplerBinding[]) {
            { .texture = SceneTarget->color, .sampler = EffectSampler },
            { .texture = SceneTarget->depth, .sampler = EffectSampler }
        }, 2);
        SDL_DrawGPUIndexedPrimitives(render_pass, 6, 1, 0, 0, 0);
        SDL_EndGPURenderPass(render_pass);
    }

    SDL_SubmitGPUCommandBuffer(cmdbuf);
    return 0;
}

static void quit(Renderer* renderer)
{
    // Release scene resources
    SDL_ReleaseGPUGraphicsPipeline(renderer->device, ScenePipeline);
    SDL_ReleaseGPUBuffer(renderer->device, SceneVertexBuffer);
    SDL_ReleaseGPUBuffer(renderer->device, SceneIndexBuffer);
    destroy_render_target(renderer->device, SceneTarget);

    // Release post-processing resources
    SDL_ReleaseGPUGraphicsPipeline(renderer->device, EffectPipeline);
    destroy_fullscreen_quad(renderer->device, EffectQuad);
    SDL_ReleaseGPUSampler(renderer->device, EffectSampler);

    renderer_quit(renderer);
}

System depth_sampler = { "depth_sampler", init, update, draw, quit };

    // Initialize our transformation matrices using cglm
    mat4 projection = GLM_MAT4_IDENTITY_INIT;
    mat4 view = GLM_MAT4_IDENTITY_INIT;
    mat4 model = GLM_MAT4_IDENTITY_INIT;

    // Creates the Shaders & Pipelines
    {
        SDL_GPUShader* sceneVertexShader = load_shader(renderer->device, "PositionColorTransform.vert", 0, 1, 0, 0);
        SDL_GPUShader* sceneFragmentShader = load_shader(renderer->device, "SolidColorDepth.frag", 0, 1, 0, 0);
        SDL_GPUShader* effectVertexShader = load_shader(renderer->device, "TexturedQuad.vert", 0, 0, 0, 0);
        SDL_GPUShader* effectFragmentShader = load_shader(renderer->device, "DepthOutline.frag", 2, 1, 0, 0);

        SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
            .target_info = {
                .num_color_targets = 1,
                .color_target_descriptions = (SDL_GPUColorTargetDescription[]) { { .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM } },
                .has_depth_stencil_target = true,
                .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM },
            .depth_stencil_state = (SDL_GPUDepthStencilState) { .enable_depth_test = true, .enable_depth_write = true, .enable_stencil_test = false, .compare_op = SDL_GPU_COMPAREOP_LESS, .write_mask = 0xFF },
            .rasterizer_state = (SDL_GPURasterizerState) { .cull_mode = SDL_GPU_CULLMODE_NONE, .fill_mode = SDL_GPU_FILLMODE_FILL, .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE },
            .vertex_input_state = (SDL_GPUVertexInputState) { .num_vertex_buffers = 1, .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]) { { .slot = 0, .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX, .instance_step_rate = 0, .pitch = sizeof(PositionColorVertex) } }, .num_vertex_attributes = 2, .vertex_attributes = (SDL_GPUVertexAttribute[]) { { .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .location = 0, .offset = 0 }, { .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, .location = 1, .offset = sizeof(float) * 3 } } },
            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
            .vertex_shader = sceneVertexShader,
            .fragment_shader = sceneFragmentShader
        };

        ScenePipeline = SDL_CreateGPUGraphicsPipeline(renderer->device, &pipelineCreateInfo);
        if (ScenePipeline == NULL) {
            SDL_Log("Failed to create Scene pipeline!");
            return -1;
        }

        pipelineCreateInfo = (SDL_GPUGraphicsPipelineCreateInfo) {
            .target_info = {
                .num_color_targets = 1,
                .color_target_descriptions = (SDL_GPUColorTargetDescription[]) { { .format = SDL_GetGPUSwapchainTextureFormat(renderer->device, renderer->window),
                    .blend_state = (SDL_GPUColorTargetBlendState) {
                        .enable_blend = true,
                        .src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
                        .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                        .color_blend_op = SDL_GPU_BLENDOP_ADD,
                        .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
                        .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                        .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
                    } } },
            },
            .vertex_input_state = (SDL_GPUVertexInputState) { .num_vertex_buffers = 1, .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]) { { .slot = 0, .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX, .instance_step_rate = 0, .pitch = sizeof(PositionTextureVertex) } }, .num_vertex_attributes = 2, .vertex_attributes = (SDL_GPUVertexAttribute[]) { { .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .location = 0, .offset = 0 }, { .buffer_slot = 0, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, .location = 1, .offset = sizeof(float) * 3 } } },
            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
            .vertex_shader = effectVertexShader,
            .fragment_shader = effectFragmentShader
        };

        EffectPipeline = SDL_CreateGPUGraphicsPipeline(renderer->device, &pipelineCreateInfo);
        if (EffectPipeline == NULL) {
            SDL_Log("Failed to create Outline Effect pipeline!");
            return -1;
        }

        SDL_ReleaseGPUShader(renderer->device, effectVertexShader);
        SDL_ReleaseGPUShader(renderer->device, effectFragmentShader);

        SDL_ReleaseGPUShader(renderer->device, sceneVertexShader);
        SDL_ReleaseGPUShader(renderer->device, sceneFragmentShader);
    }

    // Create the Scene Textures
    {
        // Make them smaller so pixels stand out more
        int w, h;
        SDL_GetWindowSizeInPixels(renderer->window, &w, &h);
        SceneWidth = w / 4;
        SceneHeight = h / 4;

        SceneColorTexture = SDL_CreateGPUTexture(
            renderer->device,
            &(SDL_GPUTextureCreateInfo) {
                .type = SDL_GPU_TEXTURETYPE_2D,
                .width = SceneWidth,
                .height = SceneHeight,
                .layer_count_or_depth = 1,
                .num_levels = 1,
                .sample_count = SDL_GPU_SAMPLECOUNT_1,
                .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
                .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET });

        SceneDepthTexture = SDL_CreateGPUTexture(
            renderer->device,
            &(SDL_GPUTextureCreateInfo) {
                .type = SDL_GPU_TEXTURETYPE_2D,
                .width = SceneWidth,
                .height = SceneHeight,
                .layer_count_or_depth = 1,
                .num_levels = 1,
                .sample_count = SDL_GPU_SAMPLECOUNT_1,
                .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
                .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET });
    }

    // Create Outline Effect Sampler
    EffectSampler = SDL_CreateGPUSampler(renderer->device, &(SDL_GPUSamplerCreateInfo) {
                                                               .min_filter = SDL_GPU_FILTER_NEAREST,
                                                               .mag_filter = SDL_GPU_FILTER_NEAREST,
                                                               .mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
                                                               .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
                                                               .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
                                                               .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
                                                           });

    // Create & Upload Scene Index and Vertex Buffers
    {
        SceneVertexBuffer = SDL_CreateGPUBuffer(
            renderer->device,
            &(SDL_GPUBufferCreateInfo) {
                .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                .size = sizeof(PositionColorVertex) * 24 });

        SceneIndexBuffer = SDL_CreateGPUBuffer(
            renderer->device,
            &(SDL_GPUBufferCreateInfo) {
                .usage = SDL_GPU_BUFFERUSAGE_INDEX,
                .size = sizeof(Uint16) * 36 });

        SDL_GPUTransferBuffer* bufferTransferBuffer = SDL_CreateGPUTransferBuffer(
            renderer->device,
            &(SDL_GPUTransferBufferCreateInfo) {
                .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                .size = (sizeof(PositionColorVertex) * 24) + (sizeof(Uint16) * 36) });

        PositionColorVertex* transferData = SDL_MapGPUTransferBuffer(
            renderer->device,
            bufferTransferBuffer,
            false);

        // Define vertices with float arrays for positions
        PositionColorVertex cube_vertices[] = {
            {{-10.0f, -10.0f, -10.0f}, 255, 0, 0, 255},   // Front face (red)
            {{10.0f, -10.0f, -10.0f}, 255, 0, 0, 255},
            {{10.0f, 10.0f, -10.0f}, 255, 0, 0, 255},
            {{-10.0f, 10.0f, -10.0f}, 255, 0, 0, 255},
            
            {{-10.0f, -10.0f, 10.0f}, 255, 255, 0, 255},  // Back face (yellow)
            {{10.0f, -10.0f, 10.0f}, 255, 255, 0, 255},
            {{10.0f, 10.0f, 10.0f}, 255, 255, 0, 255},
            {{-10.0f, 10.0f, 10.0f}, 255, 255, 0, 255},
            
            {{-10.0f, -10.0f, -10.0f}, 255, 0, 255, 255}, // Left face (magenta)
            {{-10.0f, 10.0f, -10.0f}, 255, 0, 255, 255},
            {{-10.0f, 10.0f, 10.0f}, 255, 0, 255, 255},
            {{-10.0f, -10.0f, 10.0f}, 255, 0, 255, 255},
            
            {{10.0f, -10.0f, -10.0f}, 0, 255, 0, 255},    // Right face (green)
            {{10.0f, 10.0f, -10.0f}, 0, 255, 0, 255},
            {{10.0f, 10.0f, 10.0f}, 0, 255, 0, 255},
            {{10.0f, -10.0f, 10.0f}, 0, 255, 0, 255},
            
            {{-10.0f, -10.0f, -10.0f}, 0, 255, 255, 255}, // Bottom face (cyan)
            {{-10.0f, -10.0f, 10.0f}, 0, 255, 255, 255},
            {{10.0f, -10.0f, 10.0f}, 0, 255, 255, 255},
            {{10.0f, -10.0f, -10.0f}, 0, 255, 255, 255},
            
            {{-10.0f, 10.0f, -10.0f}, 0, 0, 255, 255},    // Top face (blue)
            {{-10.0f, 10.0f, 10.0f}, 0, 0, 255, 255},
            {{10.0f, 10.0f, 10.0f}, 0, 0, 255, 255},
            {{10.0f, 10.0f, -10.0f}, 0, 0, 255, 255}
        };
        memcpy(transferData, cube_vertices, sizeof(cube_vertices));

        Uint16* indexData = (Uint16*)&transferData[24];
        Uint16 indices[] = {
            0, 1, 2, 0, 2, 3,
            4, 5, 6, 4, 6, 7,
            8, 9, 10, 8, 10, 11,
            12, 13, 14, 12, 14, 15,
            16, 17, 18, 16, 18, 19,
            20, 21, 22, 20, 22, 23
        };
        SDL_memcpy(indexData, indices, sizeof(indices));

        SDL_UnmapGPUTransferBuffer(renderer->device, bufferTransferBuffer);

        // Upload the transfer data to the GPU buffers
        SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(renderer->device);
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);

        SDL_UploadToGPUBuffer(
            copyPass,
            &(SDL_GPUTransferBufferLocation) {
                .transfer_buffer = bufferTransferBuffer,
                .offset = 0 },
            &(SDL_GPUBufferRegion) {
                .buffer = SceneVertexBuffer,
                .offset = 0,
                .size = sizeof(PositionColorVertex) * 24 },
            false);

        SDL_UploadToGPUBuffer(
            copyPass,
            &(SDL_GPUTransferBufferLocation) {
                .transfer_buffer = bufferTransferBuffer,
                .offset = sizeof(PositionColorVertex) * 24 },
            &(SDL_GPUBufferRegion) {
                .buffer = SceneIndexBuffer,
                .offset = 0,
                .size = sizeof(Uint16) * 36 },
            false);

        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(uploadCmdBuf);
        SDL_ReleaseGPUTransferBuffer(renderer->device, bufferTransferBuffer);
    }

    // Create & Upload Outline Effect Vertex and Index buffers
    {
        EffectVertexBuffer = SDL_CreateGPUBuffer(
            renderer->device,
            &(SDL_GPUBufferCreateInfo) {
                .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
                .size = sizeof(PositionTextureVertex) * 4 });

        EffectIndexBuffer = SDL_CreateGPUBuffer(
            renderer->device,
            &(SDL_GPUBufferCreateInfo) {
                .usage = SDL_GPU_BUFFERUSAGE_INDEX,
                .size = sizeof(Uint16) * 6 });

        SDL_GPUTransferBuffer* bufferTransferBuffer = SDL_CreateGPUTransferBuffer(
            renderer->device,
            &(SDL_GPUTransferBufferCreateInfo) {
                .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
                .size = (sizeof(PositionTextureVertex) * 4) + (sizeof(Uint16) * 6) });

        PositionTextureVertex* transferData = SDL_MapGPUTransferBuffer(
            renderer->device,
            bufferTransferBuffer,
            false);

        PositionTextureVertex quad_vertices[] = {
            {{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},   // Top-left
            {{1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},    // Top-right
            {{1.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},   // Bottom-right
            {{-1.0f, -1.0f, 0.0f}, {0.0f, 1.0f}}   // Bottom-left
        };
        memcpy(transferData, quad_vertices, sizeof(quad_vertices));

        Uint16* indexData = (Uint16*)&transferData[4];
        indexData[0] = 0;
        indexData[1] = 1;
        indexData[2] = 2;
        indexData[3] = 0;
        indexData[4] = 2;
        indexData[5] = 3;

        SDL_UnmapGPUTransferBuffer(renderer->device, bufferTransferBuffer);

        SDL_GPUCommandBuffer* uploadCmdBuf = SDL_AcquireGPUCommandBuffer(renderer->device);
        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuf);

        SDL_UploadToGPUBuffer(
            copyPass,
            &(SDL_GPUTransferBufferLocation) {
                .transfer_buffer = bufferTransferBuffer,
                .offset = 0 },
            &(SDL_GPUBufferRegion) {
                .buffer = EffectVertexBuffer,
                .offset = 0,
                .size = sizeof(PositionTextureVertex) * 4 },
            false);

        SDL_UploadToGPUBuffer(
            copyPass,
            &(SDL_GPUTransferBufferLocation) {
                .transfer_buffer = bufferTransferBuffer,
                .offset = sizeof(PositionTextureVertex) * 4 },
            &(SDL_GPUBufferRegion) {
                .buffer = EffectIndexBuffer,
                .offset = 0,
                .size = sizeof(Uint16) * 6 },
            false);

        SDL_EndGPUCopyPass(copyPass);
        SDL_SubmitGPUCommandBuffer(uploadCmdBuf);
        SDL_ReleaseGPUTransferBuffer(renderer->device, bufferTransferBuffer);
    }

    Time = 0;
    return 0;
}

static int update(Renderer* renderer)
{
    Time += renderer->delta_time;
    return 0;
}

static int draw(Renderer* renderer)
{
    SDL_GPUCommandBuffer* cmdbuf = SDL_AcquireGPUCommandBuffer(renderer->device);
    if (cmdbuf == NULL) {
        SDL_Log("AcquireGPUCommandBuffer failed: %s", SDL_GetError());
        return -1;
    }

    SDL_GPUTexture* swapchainTexture;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, renderer->window, &swapchainTexture, NULL, NULL)) {
        SDL_Log("WaitAndAcquireGPUSwapchainTexture failed: %s", SDL_GetError());
        return -1;
    }

    if (swapchainTexture != NULL) {
        // Render the 3D Scene (Color and Depth pass)
        float nearPlane = 20.0f;
        float farPlane = 60.0f;

        // Camera setup
        vec3 eye = { SDL_cosf(Time) * 30, 30, SDL_sinf(Time) * 30 };
        vec3 center = { 0, 0, 0 };
        vec3 up = { 0, 1, 0 };

        // Create projection and view matrices
        mat4 proj = GLM_MAT4_IDENTITY_INIT;
        mat4 view = GLM_MAT4_IDENTITY_INIT;
        mat4 viewproj = GLM_MAT4_IDENTITY_INIT;
        glm_perspective(75.0f * SDL_PI_F / 180.0f,
                       SceneWidth / (float)SceneHeight,
                       nearPlane,
                       farPlane,
                       proj);
        glm_lookat(eye, center, up, view);
        
        // Combine view and projection matrices (in correct order)
        glm_mat4_mul(proj, view, viewproj);

        SDL_GPUColorTargetInfo colorTargetInfo = { 0 };
        colorTargetInfo.texture = SceneColorTexture;
        colorTargetInfo.clear_color = (SDL_FColor) { 0.0f, 0.0f, 0.0f, 0.0f };
        colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPUDepthStencilTargetInfo depthStencilTargetInfo = { 0 };
        depthStencilTargetInfo.texture = SceneDepthTexture;
        depthStencilTargetInfo.cycle = true;
        depthStencilTargetInfo.clear_depth = 1;
        depthStencilTargetInfo.clear_stencil = 0;
        depthStencilTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
        depthStencilTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
        depthStencilTargetInfo.stencil_load_op = SDL_GPU_LOADOP_CLEAR;
        depthStencilTargetInfo.stencil_store_op = SDL_GPU_STOREOP_STORE;

        SDL_PushGPUVertexUniformData(cmdbuf, 0, viewproj, sizeof(viewproj));
        SDL_PushGPUFragmentUniformData(cmdbuf, 0, (float[]) { nearPlane, farPlane }, 8);

        SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(cmdbuf, &colorTargetInfo, 1, &depthStencilTargetInfo);
        SDL_BindGPUVertexBuffers(renderPass, 0, &(SDL_GPUBufferBinding) { .buffer = SceneVertexBuffer, .offset = 0 }, 1);
        SDL_BindGPUIndexBuffer(renderPass, &(SDL_GPUBufferBinding) { .buffer = SceneIndexBuffer, .offset = 0 }, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        SDL_BindGPUGraphicsPipeline(renderPass, ScenePipeline);
        SDL_DrawGPUIndexedPrimitives(renderPass, 36, 1, 0, 0, 0);
        SDL_EndGPURenderPass(renderPass);

        // Render the Outline Effect that samples from the Color/Depth textures
        SDL_GPUColorTargetInfo swapchainTargetInfo = { 0 };
        swapchainTargetInfo.texture = swapchainTexture;
        swapchainTargetInfo.clear_color = (SDL_FColor) { 0.2f, 0.5f, 0.4f, 1.0f };
        swapchainTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
        swapchainTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

        renderPass = SDL_BeginGPURenderPass(cmdbuf, &swapchainTargetInfo, 1, NULL);
        SDL_BindGPUGraphicsPipeline(renderPass, EffectPipeline);
        SDL_BindGPUVertexBuffers(renderPass, 0, &(SDL_GPUBufferBinding) { .buffer = EffectVertexBuffer, .offset = 0 }, 1);
        SDL_BindGPUIndexBuffer(renderPass, &(SDL_GPUBufferBinding) { .buffer = EffectIndexBuffer, .offset = 0 }, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        SDL_BindGPUFragmentSamplers(renderPass, 0, (SDL_GPUTextureSamplerBinding[]) { { .texture = SceneColorTexture, .sampler = EffectSampler }, { .texture = SceneDepthTexture, .sampler = EffectSampler } }, 2);
        SDL_DrawGPUIndexedPrimitives(renderPass, 6, 1, 0, 0, 0);
        SDL_EndGPURenderPass(renderPass);
    }

    SDL_SubmitGPUCommandBuffer(cmdbuf);

    return 0;
}

static void quit(Renderer* renderer)
{
    SDL_ReleaseGPUGraphicsPipeline(renderer->device, ScenePipeline);
    SDL_ReleaseGPUTexture(renderer->device, SceneColorTexture);
    SDL_ReleaseGPUTexture(renderer->device, SceneDepthTexture);
    SDL_ReleaseGPUBuffer(renderer->device, SceneVertexBuffer);
    SDL_ReleaseGPUBuffer(renderer->device, SceneIndexBuffer);

    SDL_ReleaseGPUGraphicsPipeline(renderer->device, EffectPipeline);
    SDL_ReleaseGPUBuffer(renderer->device, EffectVertexBuffer);
    SDL_ReleaseGPUBuffer(renderer->device, EffectIndexBuffer);
    SDL_ReleaseGPUSampler(renderer->device, EffectSampler);

    renderer_quit(renderer);
}

System depth_sampler = { "depth_sampler", init, update, draw, quit };
