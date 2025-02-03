#include "Shiv/renderer.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

bool init(Renderer* renderer) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) <= 0) {
        SDL_Log("SDL initialization failed: %s", SDL_GetError());
        return false;
    }

    // Create window
    renderer->window = SDL_CreateWindow(
        "SDL3 GPU Test",
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_RESIZABLE);

    if (!renderer->window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        return false;
    }

    // Create GPU device
    renderer->gpu_device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV
            | SDL_GPU_SHADERFORMAT_METALLIB
            | SDL_GPU_SHADERFORMAT_MSL
            | SDL_GPU_SHADERFORMAT_DXIL,
        true,
        NULL);

    if (!renderer->gpu_device) {
        SDL_Log(
            "GPU device creation failed: %s",
            SDL_GetError());
        return false;
    }

    // Claim the window for GPU rendering
    if (!SDL_ClaimWindowForGPUDevice(
            renderer->gpu_device,
            renderer->window)) {
        SDL_Log(
            "Failed to claim window for GPU: %s",
            SDL_GetError());
        return false;
    }

    renderer->running = true;
    return true;
}

void cleanup(Renderer* renderer) {
    if (renderer->window && renderer->gpu_device) {
        SDL_ReleaseWindowFromGPUDevice(
            renderer->gpu_device,
            renderer->window);
    }
    if (renderer->gpu_device) {
        SDL_DestroyGPUDevice(renderer->gpu_device);
    }
    if (renderer->window) {
        SDL_DestroyWindow(renderer->window);
    }
    SDL_Quit();
}

void handle_events(Renderer* renderer) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                renderer->running = false;
                break;
            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_ESCAPE) {
                    renderer->running = false;
                }
                break;
            default:
                break;
        }
    }
}

bool render_frame(Renderer* renderer) {
    // Get command buffer for this frame
    renderer->cmd_buffer
        = SDL_AcquireGPUCommandBuffer(renderer->gpu_device);
    if (!renderer->cmd_buffer) {
        SDL_Log(
            "Failed to acquire command buffer: %s",
            SDL_GetError());
        return false;
    }

    // Get swapchain texture
    Uint32 width, height;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(
            renderer->cmd_buffer,
            renderer->window,
            &renderer->swapchain_texture,
            &width,
            &height)) {
        SDL_Log(
            "Failed to acquire swapchain texture: %s",
            SDL_GetError());
        return false;
    }

    // Begin render pass
    SDL_GPUColorTargetInfo color_target = {
        .texture  = renderer->swapchain_texture,
        .load_op  = SDL_GPU_LOADOP_CLEAR,
        .store_op = SDL_GPU_STOREOP_DONT_CARE,
        .clear_color
        = {0.2f, 0.3f, 0.8f, 1.0f},  // Nice blue color
    };

    SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(
        renderer->cmd_buffer,
        &color_target,
        1,
        NULL);
    if (!render_pass) {
        SDL_Log(
            "Failed to begin render pass: %s",
            SDL_GetError());
        return false;
    }

    // End render pass
    SDL_EndGPURenderPass(render_pass);

    // Submit command buffer
    if (!SDL_SubmitGPUCommandBuffer(renderer->cmd_buffer)) {
        SDL_Log(
            "Failed to submit command buffer: %s",
            SDL_GetError());
        return false;
    }

    return true;
}

int render() {
    Renderer renderer = {0};

    if (!init(&renderer)) {
        cleanup(&renderer);
        return 1;
    }

    // Main loop
    while (renderer.running) {
        handle_events(&renderer);

        if (!render_frame(&renderer)) {
            SDL_Log("Render frame failed!");
            break;
        }

        SDL_Delay(16);  // Cap to roughly 60 FPS
    }

    cleanup(&renderer);
    return 0;
}