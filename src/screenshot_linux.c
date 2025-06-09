#include "screenshot.h"

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static bool run_command(char* exe, ...)
{
    char* args_array[256] = { exe };
    int args_array_count = 1;

    va_list args;
    va_start(args, exe);

    while (true) {
        char* arg = va_arg(args, char*);
        if (!arg)
            break;
        args_array[args_array_count++] = arg;
    }
    args_array[args_array_count++] = NULL;

    va_end(args);

    pid_t process = fork();
    if (process == 0) {
        if (execvp(exe, args_array) == -1) {
            fprintf(stderr, "ERROR: could not execute `%s`: %s\n",
                    exe, strerror(errno));
            exit(1);
        }
        exit(0);
    }

    int process_status = 0;
    if (waitpid(process, &process_status, 0) == -1) {
        fprintf(stderr, "ERROR: could not `waitpid`: %s\n", strerror(errno));
        return false;
    }

    if (!WIFEXITED(process_status)) {
        fprintf(stderr, "ERROR: `%s` subprocess exited abnormally\n", exe);
        return false;
    }

    int process_return_code = WEXITSTATUS(process_status);
    if (process_return_code != 0) {
        fprintf(stderr, "ERROR: `%s` subprocess exited with code `%d`\n",
                exe, process_return_code);
        return false;
    }

    return true;
}

static bool take_screenshot_spectacle(const char* screenshot_file_path)
{
    return run_command("spectacle", "-b", "-n", "-o", screenshot_file_path, NULL);
}

static bool take_screenshot_grim(const char* screenshot_file_path)
{
    return run_command("grim", screenshot_file_path, NULL);
}

static bool take_screenshot_x11(const char* screenshot_file_path)
{
    return run_command("scrot", "-o", "-F", screenshot_file_path, NULL);
}

bool take_screenshot(const char* screenshot_file_path)
{
    if (getenv("WAYLAND_DISPLAY") == NULL)
        return take_screenshot_x11(screenshot_file_path);

    char const* xdg_current_desktop = getenv("XDG_CURRENT_DESKTOP");
    if (xdg_current_desktop == NULL)
        return take_screenshot_grim(screenshot_file_path);

    if (strcmp(xdg_current_desktop, "KDE") == 0)
        return take_screenshot_spectacle(screenshot_file_path);

    return take_screenshot_grim(screenshot_file_path);
}
