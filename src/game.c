#include "game_shared.h"
#include "../modules/creload/cr.h"
#include <stdbool.h>

#define WIDTH 1280
#define HEIGHT 800

// Helper functions
static struct color {
    float r, g, b, a;
} color;

static float hue_to_rgb(float p, float q, float t) {
    if (t < 0.0f) t += 1.0f;
    if (t > 1.0f) t -= 1.0f;
    if (t < 1.0f/6.0f) return p + (q - p) * 6.0f * t;
    if (t < 1.0f/2.0f) return q;
    if (t < 2.0f/3.0f) return p + (q - p) * (2.0f/3.0f - t) * 6.0f;
    return p;
}

static struct color color_from_hsl(float hue, float saturation, float lightness) {
    hue = hue / 360.0f;
    saturation = saturation / 100.0f;
    lightness = lightness / 100.0f;
    
    if (saturation == 0.0f) {
        return (struct color){lightness, lightness, lightness, 1.0f};
    } else {
        const float q = lightness < 0.5f 
            ? lightness * (1.0f + saturation)
            : lightness + saturation - lightness * saturation;
        const float p = 2.0f * lightness - q;
        return (struct color){
            hue_to_rgb(p, q, hue + 1.0f/3.0f),
            hue_to_rgb(p, q, hue),
            hue_to_rgb(p, q, hue - 1.0f/3.0f),
            1.0f
        };
    }
}

// Static game state that persists between reloads
static struct game_context CR_STATE game_ctx = {0};

// Plugin entry point
CR_EXPORT int cr_main(struct cr_plugin *ctx, enum cr_op operation) {
    // Cast the userdata to our game context
    struct game_context *host_ctx = (struct game_context *)ctx->userdata;
    
    switch (operation) {
        case CR_LOAD: {
            // Copy data from host
            game_ctx = *host_ctx;
            return 0;
        }
        
        case CR_UNLOAD: {
            // Save state back to host
            *host_ctx = game_ctx;
            return 0;
        }
        
        case CR_CLOSE: {
            return 0;
        }
        
        case CR_STEP: {
            SDL_Event event = {0};
            
            // Process events
            while (game_ctx.sym->SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_KEY_DOWN && event.key.repeat == 0) {
                    if (event.key.scancode == SDL_SCANCODE_Q) {
                        game_ctx.running = false;
                    }
                }
                if (event.type == SDL_EVENT_QUIT) {
                    game_ctx.running = false;
                }
            }

            // Update color
            game_ctx.hue += 0.5f;
            if (game_ctx.hue >= 360.0f) {
                game_ctx.hue = 0.0f;
            }

            // Render frame
            struct color c = color_from_hsl(game_ctx.hue, 100, 50);
            game_ctx.sym->SDL_SetRenderDrawColor(game_ctx.renderer, 
                c.r * 255, c.g * 255, c.b * 255, 128);
            game_ctx.sym->SDL_RenderClear(game_ctx.renderer);
            game_ctx.sym->SDL_RenderPresent(game_ctx.renderer);
            game_ctx.sym->SDL_UpdateWindowSurface(game_ctx.window);
            game_ctx.sym->SDL_Delay(16);

            // Save state back to host context
            *host_ctx = game_ctx;

            return game_ctx.running;
        }
    }
    
    return 0;
}
