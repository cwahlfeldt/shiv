#include <SDL3/SDL_main.h>
#include <shiv/shiv.h>

static System* Systems[] = { &depth_sampler };

int main(int argc, char** argv)
{
    Renderer renderer = { 0 };
    int example_index = -1;
    int goto_system_index = 0;
    int quitting = 0;
    float last_time = 0;

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    initialize_asset_loader();

    bool canDraw = true;

    while (!quitting) {
        SDL_Event evt;
        while (SDL_PollEvent(&evt)) {
            if (evt.type == SDL_EVENT_QUIT) {
                if (example_index != -1) {
                    Systems[example_index]->quit(&renderer);
                }
                quitting = 1;
            }
        }
        if (quitting) {
            break;
        }

        if (goto_system_index != -1) {
            if (example_index != -1) {
                Systems[example_index]->quit(&renderer);
                SDL_zero(renderer);
            }

            example_index = goto_system_index;
            renderer.name = Systems[example_index]->name;
            SDL_Log("STARTING EXAMPLE: %s", renderer.name);
            if (Systems[example_index]->init(&renderer) < 0) {
                SDL_Log("Init failed!");
                return 1;
            }

            goto_system_index = -1;
        }

        float new_time = SDL_GetTicks() / 1000.0f;
        renderer.delta_time = new_time - last_time;
        last_time = new_time;

        if (Systems[example_index]->update(&renderer) < 0) {
            SDL_Log("Update failed!");
            return 1;
        }

        if (canDraw) {
            if (Systems[example_index]->draw(&renderer) < 0) {
                SDL_Log("Draw failed!");
                return 1;
            }
        }
    }

    return 0;
}
