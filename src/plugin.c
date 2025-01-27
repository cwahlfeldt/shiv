#define _COSMO_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "shiv_plugin.h"
#include "shiv_sdl.h"
#include "shiv_color.h"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <read_fd> <write_fd>\n", argv[0]);
        return 1;
    }

    int read_fd = atoi(argv[1]);
    int write_fd = atoi(argv[2]);

    // Set pipe fds to non-blocking
    int flags;
    flags = fcntl(read_fd, F_GETFL, 0);
    fcntl(read_fd, F_SETFL, flags | O_NONBLOCK);
    flags = fcntl(write_fd, F_GETFL, 0);
    fcntl(write_fd, F_SETFL, flags | O_NONBLOCK);

    struct plugin_message msg;
    msg.data.state.hue = 0.0f;
    msg.data.state.running = true;

    printf("Plugin starting...\n");
    
    // Main plugin loop
    while (msg.data.state.running) {
        // Read any incoming messages
        int bytes = read(read_fd, &msg, sizeof(msg));
        if (bytes > 0) {
            printf("Plugin received message type: %d (MSG_UPDATE=%d)\n", msg.type, MSG_UPDATE);
            if (msg.type == MSG_UPDATE) {
                printf("Plugin: processing update message\n");
                // Update game state
                msg.data.state.hue += 0.5f;
                if (msg.data.state.hue >= 360.0f)
                    msg.data.state.hue = 0.0f;

                printf("Plugin: hue updated to %f\n", msg.data.state.hue);

                // Send state back
                msg.type = MSG_STATE;
                write(write_fd, &msg, sizeof(msg));

                // Send render command
                struct color c = color_from_hsl(msg.data.state.hue, 100, 50);
                printf("Plugin: color computed r=%f g=%f b=%f\n", c.r, c.g, c.b);
                msg.type = MSG_RENDER;
                msg.data.color = c;  // We need to add this to the message structure
                write(write_fd, &msg, sizeof(msg));
            }
        } else if (bytes < 0 && errno != EAGAIN) {
            printf("Plugin: read error %d\n", errno);
            break;
        }
        usleep(16000); // ~60 FPS
    }

    return 0;
}
