#include "command.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "wish.h"
#include "utils.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

// Forward declarations for helpers
static int handle_builtin(char **argv);

// Handle built-in commands: exit, cd, path
static int handle_builtin(char **argv) {
    if (!argv || !argv[0]) return 0;
    if (strcmp(argv[0], "exit") == 0) {
        if (argv[1] != NULL) {
            shell_error(E2BIG);
            return 1;
        }
        exit(0);
    } else if (strcmp(argv[0], "cd") == 0) {
        if (!argv[1] || argv[2]) {
            shell_error(EINVAL);
            return 1;
        }
        if (chdir(argv[1]) != 0) {
            shell_error(ENOENT);
        }
        return 1;
    } else if (strcmp(argv[0], "path") == 0) {
        // Reset shell_paths
        for (int i = 0; i < shell_path_count; i++) {
            free(shell_paths[i]);
        }
        free(shell_paths);
        shell_path_count = 0;
        int n = 0;
        while (argv[1 + n]) n++;
        if (n > 0) {
            shell_paths = malloc(sizeof(char*) * n);
            for (int i = 0; i < n; i++) {
                shell_paths[i] = strdup(argv[1 + i]);
            }
            shell_path_count = n;
        } else {
            shell_paths = NULL;
        }
        return 1;
    }
    return 0;
}

// Helper to execute a single command (handles redirection)
static int execute_single_command(char *cmd_str) {
    if (!cmd_str || *cmd_str == '\0') return 0; // Empty command

    char *redir_target = NULL;
    if (parse_redirection(cmd_str, &redir_target) != 0) {
        shell_error(EINVAL);
        return -1;
    }

    char *tokens[512 / 2 + 1];
    char *cmd_copy = strdup(cmd_str);
    if (!cmd_copy) {
        if (redir_target) free(redir_target);
        shell_error(ENOMEM);
        return -1;
    }

    int token_count = tokenize_input(cmd_copy, tokens, 512 / 2 + 1);
    if (token_count == 0) {
        free(cmd_copy);
        if (redir_target) free(redir_target);
        return 0;
    }
    if (handle_builtin(tokens)) {
        free(cmd_copy);
        if (redir_target) free(redir_target);
        return 0;
    }
    if (shell_path_count == 0) {
        shell_error(ENOENT);
        free(cmd_copy);
        if (redir_target) free(redir_target);
        return -1;
    }
    char fullpath[1024];
    int found = 0;
    if (tokens[0][0] == '/' || (tokens[0][0] == '.' && tokens[0][1] == '/')) {
        if (access(tokens[0], X_OK) == 0) {
            strncpy(fullpath, tokens[0], sizeof(fullpath) - 1);
            fullpath[sizeof(fullpath) - 1] = '\0';
            found = 1;
        }
    } else {
        for (int i = 0; i < shell_path_count; i++) {
            snprintf(fullpath, sizeof(fullpath), "%s/%s", shell_paths[i], tokens[0]);
            if (access(fullpath, X_OK) == 0) {
                found = 1;
                break;
            }
        }
    }
    if (!found) {
        shell_error(ENOENT);
        free(cmd_copy);
        if (redir_target) free(redir_target);
        return -1;
    }
    pid_t pid = fork();
    if (pid < 0) {
        shell_error(EAGAIN);
        free(cmd_copy);
        if (redir_target) free(redir_target);
        return -1;
    } else if (pid == 0) {
        if (redir_target) {
            int fd = open(redir_target, O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (fd < 0) {
                shell_error(EACCES);
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }
        execv(fullpath, tokens);
        shell_error(ENOEXEC);
        exit(1);
    }
    free(cmd_copy);
    if (redir_target) free(redir_target);
    return 0;
}

// Implementation of split_parallel_commands
char **split_parallel_commands(char *linecopy, int *out_count) {
    size_t cap = 8, cnt = 0;
    char **parts = malloc(sizeof(char*) * cap);
    if (!parts) {
        shell_error(ENOMEM);
        return NULL;
    }
    char *p = linecopy;
    while (1) {
        char *amp = strchr(p, '&');
        if (!amp) {
            char *part = trim_whitespace(p);
            if (*part != '\0') {
                if (cnt >= cap) {
                    cap *= 2;
                    char **new_parts = realloc(parts, sizeof(char*) * cap);
                    if (!new_parts) {
                        free(parts);
                        shell_error(ENOMEM);
                        return NULL;
                    }
                    parts = new_parts;
                }
                parts[cnt++] = part;
            }
            break;
        }
        *amp = '\0';
        char *part = trim_whitespace(p);
        if (*part != '\0') {
            if (cnt >= cap) {
                cap *= 2;
                char **new_parts = realloc(parts, sizeof(char*) * cap);
                if (!new_parts) {
                    free(parts);
                    shell_error(ENOMEM);
                    return NULL;
                }
                parts = new_parts;
            }
            parts[cnt++] = part;
        }
        p = amp + 1;
    }
    parts[cnt] = NULL;
    if (out_count) *out_count = cnt;
    return parts;
}

// Implementation of parse_redirection
int parse_redirection(char *cmd, char **out_target) {
    char *redir = strchr(cmd, '>');
    if (!redir) {
        if (out_target) *out_target = NULL;
        return 0;
    }
    *redir = '\0';
    redir++;
    while (*redir == ' ' || *redir == '\t') redir++;
    if (*redir == '\0') return -1;
    char *end = redir;
    while (*end && *end != ' ' && *end != '\t') end++;
    if (*end) *end = '\0';
    if (out_target) *out_target = strdup(redir);
    return 1;
}

// Implementation of process_command_line (stub, should be filled in with actual logic)
int process_command_line(char *line) {
    if (!line || *line == '\0') return 0;
    char *linecopy = strdup(line);
    if (!linecopy) {
        shell_error(ENOMEM);
        return -1;
    }
    int cmd_count = 0;
    char **cmds = split_parallel_commands(linecopy, &cmd_count);
    if (!cmds) {
        free(linecopy);
        return -1;
    }
    pid_t *pids = malloc(sizeof(pid_t) * cmd_count);
    if (!pids) {
        free(cmds);
        free(linecopy);
        shell_error(ENOMEM);
        return -1;
    }
    int pid_count = 0;
    for (int i = 0; i < cmd_count; i++) {
        if (cmds[i] && *cmds[i] != '\0') {
            char *cmd_str = cmds[i];
            char *redir_target = NULL;
            if (parse_redirection(cmd_str, &redir_target) != 0) {
                shell_error(EINVAL);
                if (redir_target) free(redir_target);
                continue;
            }
            char *tokens[512 / 2 + 1];
            char *cmd_copy = strdup(cmd_str);
            if (!cmd_copy) {
                if (redir_target) free(redir_target);
                shell_error(ENOMEM);
                continue;
            }
            int token_count = tokenize_input(cmd_copy, tokens, 512 / 2 + 1);
            if (token_count == 0) {
                free(cmd_copy);
                if (redir_target) free(redir_target);
                continue;
            }
            if (handle_builtin(tokens)) {
                free(cmd_copy);
                if (redir_target) free(redir_target);
                continue;
            }
            if (shell_path_count == 0) {
                shell_error(ENOENT);
                free(cmd_copy);
                if (redir_target) free(redir_target);
                continue;
            }
            char fullpath[1024];
            int found = 0;
            if (tokens[0][0] == '/' || (tokens[0][0] == '.' && tokens[0][1] == '/')) {
                if (access(tokens[0], X_OK) == 0) {
                    strncpy(fullpath, tokens[0], sizeof(fullpath) - 1);
                    fullpath[sizeof(fullpath) - 1] = '\0';
                    found = 1;
                }
            } else {
                for (int j = 0; j < shell_path_count; j++) {
                    snprintf(fullpath, sizeof(fullpath), "%s/%s", shell_paths[j], tokens[0]);
                    if (access(fullpath, X_OK) == 0) {
                        found = 1;
                        break;
                    }
                }
            }
            if (!found) {
                shell_error(ENOENT);
                free(cmd_copy);
                if (redir_target) free(redir_target);
                continue;
            }
            pid_t pid = fork();
            if (pid < 0) {
                shell_error(EAGAIN);
                free(cmd_copy);
                if (redir_target) free(redir_target);
                continue;
            } else if (pid == 0) {
                if (redir_target) {
                    fprintf(stderr, "[DEBUG] Attempting to open redirection target: %s\n", redir_target);
                    int fd = open(redir_target, O_CREAT | O_WRONLY | O_TRUNC, 0644);
                    if (fd < 0) {
                        fprintf(stderr, "[DEBUG] Failed to open redirection file: %s\n", redir_target);
                        shell_error(EACCES);
                        exit(1);
                    }
                    fprintf(stderr, "[DEBUG] Successfully opened %s (fd=%d). Redirecting stdout and stderr.\n", redir_target, fd);
                    dup2(fd, STDOUT_FILENO);
                    dup2(fd, STDERR_FILENO);
                    close(fd);
                }
                fprintf(stderr, "[DEBUG] Executing command: %s\n", fullpath);
                execv(fullpath, tokens);
                fprintf(stderr, "[DEBUG] execv failed for %s\n", fullpath);
                shell_error(ENOEXEC);
                exit(1);
            } else {
                pids[pid_count++] = pid;
            }
            free(cmd_copy);
            if (redir_target) free(redir_target);
        }
    }
    for (int i = 0; i < pid_count; i++) {
        waitpid(pids[i], NULL, 0);
    }
    free(pids);
    free(cmds);
    free(linecopy);
    return 0;
}
