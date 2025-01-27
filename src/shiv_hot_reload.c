#include "shiv_hot_reload.h"
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>

#ifdef SHIV_HOT_RELOAD

static time_t get_file_mtime(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return st.st_mtime;
    }
    return 0;
}

static void cleanup_plugin(struct plugin_reload_data* data) {
    if (data->pid > 0) {
        kill(data->pid, SIGTERM);
        waitpid(data->pid, NULL, 0);
        data->pid = 0;
    }

    // Close all pipes
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            if (data->pipes[i][j] > 0) {
                close(data->pipes[i][j]);
                data->pipes[i][j] = 0;
            }
        }
    }
}

static bool launch_plugin(struct plugin_reload_data* data) {
    // Create pipes
    if (pipe(data->pipes[0]) == -1 || pipe(data->pipes[1]) == -1) {
        return false;
    }

    // Convert FDs to strings for args
    char read_fd[16], write_fd[16];
    snprintf(read_fd, sizeof(read_fd), "%d", data->pipes[0][0]);
    snprintf(write_fd, sizeof(write_fd), "%d", data->pipes[1][1]);

    char* const args[] = {(char*)data->plugin_path, read_fd, write_fd, NULL};

    pid_t pid = fork();
    if (pid == -1) {
        return false;
    }

    if (pid == 0) {  // Child process
        execv(data->plugin_path, args);
        _exit(1);  // If execv fails
    }

    // Parent process
    data->pid = pid;

    // Close unused ends of pipes
    close(data->pipes[0][0]);
    close(data->pipes[1][1]);

    // Set remaining pipe ends to non-blocking
    int flags;
    flags = fcntl(data->pipes[0][1], F_GETFL, 0);
    fcntl(data->pipes[0][1], F_SETFL, flags | O_NONBLOCK);
    flags = fcntl(data->pipes[1][0], F_GETFL, 0);
    fcntl(data->pipes[1][0], F_SETFL, flags | O_NONBLOCK);

    // Store initial modification time
    data->last_modified = get_file_mtime(data->plugin_path);
    return true;
}

bool hot_reload_init(struct hot_reload_handler* handler) {
    if (!handler || !handler->data) return false;
    
    struct plugin_reload_data* data = (struct plugin_reload_data*)handler->data;
    return launch_plugin(data);
}

bool hot_reload_check(struct hot_reload_handler* handler) {
    if (!handler || !handler->data) return false;

    struct plugin_reload_data* data = (struct plugin_reload_data*)handler->data;
    time_t current_mtime = get_file_mtime(data->plugin_path);
    
    if (current_mtime <= data->last_modified) {
        return true;
    }

    cleanup_plugin(data);
    
    // Small delay to ensure file is fully written
    usleep(100000);
    
    return launch_plugin(data);
}

void hot_reload_cleanup(struct hot_reload_handler* handler) {
    if (!handler || !handler->data) return;
    
    struct plugin_reload_data* data = (struct plugin_reload_data*)handler->data;
    cleanup_plugin(data);
}

#endif // SHIV_HOT_RELOAD