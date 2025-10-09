#include <stddef.h>
#include <stdio.h>
#include <errno.h> // Error handling library. Assigns errno variable with error code when they occur.
#include <string.h> // For strerror()
#include <stdlib.h>
#include <dirent.h> // For directory operations
#include <sys/stat.h> // For file stat operations
#include <unistd.h> // For access() function
#include <sys/types.h> 
#include <sys/wait.h> // For waitpid()
#include <limits.h> // For PATH_MAX

#include <fcntl.h> // For open(), O_CREAT, O_WRONLY, O_TRUNC

// Shell's search path (dynamic)
static char **shell_paths = NULL;
static int shell_path_count = 0;

#define BUFFER_SIZE 512

// Splits a line into tokens separated by spaces/tabs.
// Returns the number of tokens parsed.
int tokenize_input(char *input, char **tokens, int max_tokens) {
    int token_count = 0;
    char *saveptr;
    char *token = strtok_r(input, " \t", &saveptr);

    while (token != NULL && token_count < max_tokens - 1) {
        tokens[token_count++] = token;
        token = strtok_r(NULL, " \t", &saveptr);
    }
    tokens[token_count] = NULL; // Null-terminate the list

    return token_count;
}

#include <unistd.h>  /* for write(), STDERR_FILENO */
#include <string.h>  /* for strcmp */
#include <stdlib.h>  /* for malloc/realloc/free */

static inline void _shell_error_msg(void) {
    const char msg[] = "An error has occurred\n";
    write(STDERR_FILENO, msg, sizeof(msg) - 1);
}

static int handle_builtin(char **argv) {
    if (!argv || !argv[0]) return 0;

    /* --- exit: takes zero args --- */
    if (strcmp(argv[0], "exit") == 0) {
        if (argv[1] != NULL) {
            _shell_error_msg();   /* exit with args -> shell error, but do not exit */
            return 1;             /* handled (error) */
        } else {
            /* Normal termination of shell. We intentionally do not try to free every
               allocation here — the OS will reclaim memory on process exit. If you
               want to free available_programs, make it global and free it here. */
            exit(0);
        }
    }

    /* --- cd: takes exactly one arg --- */
    if (strcmp(argv[0], "cd") == 0) {
        /* argc must be exactly 2 (argv[0] + one arg) */
        if (!argv[1] || argv[2] != NULL) {
            _shell_error_msg();
            return 1; /* handled (error) */
        }
        if (chdir(argv[1]) != 0) {
            /* chdir failed -> print the required single error message */
            _shell_error_msg();
        }
        return 1; /* handled successfully (or printed error) */
    }

    /* --- path: overwrite shell_paths with argv[1..] --- */
    if (strcmp(argv[0], "path") == 0) {
        /* free existing path entries */
        for (int i = 0; i < shell_path_count; ++i) {
            free(shell_paths[i]);
        }
        free(shell_paths);
        shell_paths = NULL;
        shell_path_count = 0;

        /* Count new entries (argv[1], argv[2], ... until NULL) */
        int n = 0;
        while (argv[1 + n]) ++n;

        if (n == 0) {
            /* Path set to empty: valid — no executables available (per spec) */
            shell_paths = NULL;
            shell_path_count = 0;
            return 1;
        }

        shell_paths = malloc(sizeof(char*) * n);
        if (!shell_paths) {
            _shell_error_msg();
            shell_path_count = 0;
            return 1;
        }
        for (int i = 0; i < n; ++i) {
            shell_paths[i] = strdup(argv[1 + i]);
            if (!shell_paths[i]) {
                /* allocation failure: clean up and report error */
                for (int j = 0; j < i; ++j) free(shell_paths[j]);
                free(shell_paths);
                shell_paths = NULL;
                shell_path_count = 0;
                _shell_error_msg();
                return 1;
            }
        }
        shell_path_count = n;
        return 1;
    }

    /* Not a builtin */
    return 0;
}

// BUFFER_SIZE is defined as a macro at the top

// Prints the current errno value and its descr iption
// Use this function to throw an explained error without breaking out of the loop
void print_errno() {
    printf("An error has occurred. %s (Code: %d)\n", strerror(errno), errno);
}

// Structure to hold the array of programs
typedef struct {
    char **programs;
    int count;
    int capacity;
} ProgramArray;

// Function to check if a file is executable
int is_executable(const char *filepath) {
    struct stat st; // st is a struct that holds information about the file
    if (stat(filepath, &st) == 0) { // stat() returns 0 on success. Success means file exists.
        // Check if execute permission is set (user, group, or others)
        return (st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH);
    }
    return 0;
}

// Function to add a program to the array
int add_program(ProgramArray *arr, const char *program) {
    if (arr->count >= arr->capacity) {
        // Resize array if needed
        arr->capacity *= 2;
        // new_programs is scaled to the new capacity by multiplying by sizeof(char*) and the new capacity
        char **new_programs = realloc(arr->programs, arr->capacity * sizeof(char*));
        if (!new_programs) {
            errno = ENOMEM; // bbnomem
            return -1;
        }
        arr->programs = new_programs;
    }
    
    // Allocate memory for the program name
    arr->programs[arr->count] = malloc(strlen(program) + 1);
    if (!arr->programs[arr->count]) {
        errno = ENOMEM; // someone said its a bad idea to set this value explicitly. If you find a better way, go for it.
        return -1;
    }
    
    strcpy(arr->programs[arr->count], program);
    arr->count++;
    return 0;
}

// Function to scan a directory for executable programs
int scan_bin_directory(ProgramArray *arr, const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        return -1; // Directory doesn't exist or can't be opened
    }
    
    struct dirent *entry;
    // Use dynamic allocation for filepath to avoid truncation
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and .. entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Calculate required buffer size for full path
        size_t path_len = strlen(dir_path) + 1 + strlen(entry->d_name) + 1; // '/' + '\0'
        char *filepath = malloc(path_len);
        if (!filepath) {
            closedir(dir);
            errno = ENOMEM;
            return -1;
        }
        int written = snprintf(filepath, path_len, "%s/%s", dir_path, entry->d_name);
        if (written < 0 || (size_t)written >= path_len) {
            // Truncation or error occurred
            free(filepath);
            continue;
        }

        // Check if it's a regular file and executable
        struct stat st;
        if (stat(filepath, &st) == 0 && S_ISREG(st.st_mode) && is_executable(filepath)) {
            if (add_program(arr, entry->d_name) != 0) {
                free(filepath);
                closedir(dir);
                return -1;
            }
        }
        free(filepath);
    }
    
    closedir(dir);
    return 0;
}

// Function to get all programs from common bin directories
ProgramArray* get_all_programs() {
    ProgramArray *programs = malloc(sizeof(ProgramArray));
    if (!programs) {
        errno = ENOMEM;
        return NULL;
    }
    
    // Initialize the array
    programs->capacity = 100;
    programs->count = 0;
    programs->programs = malloc(programs->capacity * sizeof(char*));
    if (!programs->programs) {
        free(programs);
        errno = ENOMEM;
        return NULL;
    }
    
    // Common bin directories to scan
    const char *bin_dirs[] = {
        "/bin",
        "/usr/bin",
        "/usr/local/bin",
        "/sbin",
        "/usr/sbin",
        NULL
    };
    
    // Scan each directory
    for (int i = 0; bin_dirs[i] != NULL; i++) {
        scan_bin_directory(programs, bin_dirs[i]);
        // Continue even if one directory fails
    }
    
    return programs;
}

// Function to free the program array
void free_program_array(ProgramArray *arr) {
    if (arr) {
        for (int i = 0; i < arr->count; i++) {
            free(arr->programs[i]);
        }
        free(arr->programs);
        free(arr);
    }
}

/* Trim leading and trailing spaces/tabs/newlines in-place.
 * Returns pointer to the first non-whitespace char (may be same pointer).
 * Modifies the string to terminate after the last non-whitespace char.
 */
static char *trim_whitespace(char *s) {
    if (!s) return s;
    // skip leading whitespace
    while (*s == ' ' || *s == '\t' || *s == '\n') s++;
    if (*s == '\0') return s;
    // trim trailing
    char *end = s + strlen(s) - 1;
    while (end > s && (*end == ' ' || *end == '\t' || *end == '\n')) {
        *end = '\0';
        end--;
    }
    return s;
}

/* Split input line into command parts on '&'.
 * - linecopy must be a writable copy (e.g., strdup(inputBuffer))
 * - returns a malloc'd array (NULL-terminated) of pointers into linecopy
 * - out_count will be set to number of parts
 * Caller must free the returned array and the original linecopy.
 */
static char **split_parallel_commands(char *linecopy, int *out_count) {
    size_t cap = 8, cnt = 0;
    char **parts = malloc(sizeof(char*) * cap);
    if (!parts) { _shell_error_msg(); return NULL; }

    char *p = linecopy;
    while (1) {
        char *amp = strchr(p, '&');
        if (!amp) {
            char *part = trim_whitespace(p);
            if (cnt >= cap) { cap *= 2; parts = realloc(parts, sizeof(char*) * cap); }
            parts[cnt++] = part;
            break;
        }
        *amp = '\0';
        char *part = trim_whitespace(p);
        if (cnt >= cap) { cap *= 2; parts = realloc(parts, sizeof(char*) * cap); }
        parts[cnt++] = part;
        p = amp + 1;
    }
    parts = realloc(parts, sizeof(char*) * (cnt + 1));
    parts[cnt] = NULL;
    if (out_count) *out_count = (int)cnt;
    return parts;
}

/* Parse a single '>' redirection in-place within cmd.
 * - cmd is mutable and will be truncated so it contains only the command portion.
 * - On success, *out_target will be set to a malloc'd filename (caller frees it) or NULL if no redirection.
 * - Returns 0 on success, -1 on syntax error.
 *
 * Accepts forms: "cmd > file", "cmd>file", "cmd> file", etc.
 * Rejects: multiple '>' or missing filename or extra tokens after filename.
 */
/* parse_redirection:
 * - cmd: mutable command string (will be truncated before '>' if redirection present)
 * - out_target: set to malloc'd filename string on success, or NULL if no redirection
 * Returns 0 on success, -1 on syntax error.
 *
 * Rules enforced:
 *  - only one '>' allowed
 *  - '>' cannot be the first non-whitespace token
 *  - exactly one filename token must follow '>' and nothing else (except whitespace)
 */
static int parse_redirection(char *cmd, char **out_target) {
    *out_target = NULL;
    if (!cmd) return 0;

    /* find first '>' */
    char *first = strchr(cmd, '>');
    if (!first) return 0; /* no redirection */

    /* ensure only one '>' occurs */
    if (strchr(first + 1, '>')) return -1;

    /* Ensure there's something before '>' (non-whitespace) */
    /* Temporarily copy left side to test */
    char *left = cmd;
    while (*left == ' ' || *left == '\t') left++;
    if (left == first) return -1; /* '>' is first non-whitespace token */

    /* Now isolate RHS and check filename rules */
    char *rhs = first + 1;
    /* skip whitespace */
    while (*rhs == ' ' || *rhs == '\t') rhs++;
    if (*rhs == '\0') return -1; /* missing filename */

    /* filename is until next whitespace (space/tab/newline) */
    char *p = rhs;
    while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\n') p++;

    /* if there's more non-whitespace after the filename -> syntax error */
    if (*p != '\0') {
        char *after = p;
        while (*after == ' ' || *after == '\t' || *after == '\n') after++;
        if (*after != '\0') return -1; /* extra token after filename */
        /* otherwise terminate filename */
        *p = '\0';
    }

    /* Now truncate cmd at the '>' (remove right side) */
    *first = '\0';
    /* Trim trailing whitespace on left side (optional) */
    /* move trimmed left to front if needed */
    char *left_trim = left;
    char *end = left_trim + strlen(left_trim) - 1;
    while (end > left_trim && (*end == ' ' || *end == '\t' || *end == '\n')) { *end = '\0'; end--; }
    if (left_trim != cmd) memmove(cmd, left_trim, strlen(left_trim) + 1);

    /* Duplicate filename for caller */
    *out_target = strdup(rhs);
    if (!*out_target) return -1; /* treat allocation failure as syntax-like error */

    return 0;
}
// Fork the current process and run parameter program in child process. Awaits completion.
void fork_and_run(const char *program, char *const argv[]) {
    pid_t pid = fork();
    if (pid < 0) {
        // Fork failed
        print_errno();
        return;
    } else if (pid == 0) {
        // Child process
        execvp(program, argv);
        // If execvp returns, an error occurred
        print_errno();
        exit(1);
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
    }
}

// Clears the standard input buffer, effectively cin.ignore()
void clear_stdin_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}


int main(int argc, char *argv[]) {
    char inputBuffer[BUFFER_SIZE]; // Buffer to hold user input
    int is_interactive = 1; // Flag for interactive mode
    FILE *infile = stdin; // Input stream (stdin for interactive, batch file for batch mode)

    ProgramArray *available_programs = get_all_programs();

    // Initialize shell path with default: /bin
    shell_path_count = 1;
    shell_paths = malloc(sizeof(char*));
    shell_paths[0] = strdup("/bin");


    if (available_programs) {
        printf("Found %d programs in system bin directories.\n", available_programs->count);
    } else {
        printf("Failed to load programs from bin directories.\n");
        print_errno();
    }

    /* Argument validation per spec:
     * - If more than one command-line argument is provided, print the single error
     *   message to stderr and exit(1).
     * - If one argument is provided, try to open it as batch file later (see below).
     */
    if (argc > 2) {
        write(STDERR_FILENO, "An error has occurred\n", 22);
        if (available_programs) free_program_array(available_programs);
        exit(1);
    }

    /* If batch mode, try opening the provided file. If fopen fails, print the
     * single error message and exit(1). (This matches the spec's requirement.)
     */
    if (argc == 2) {
        /* attempt to open batch file now so we can error-exit immediately on bad file */
        FILE *batch = fopen(argv[1], "r");
        if (!batch) {
            write(STDERR_FILENO, "An error has occurred\n", 22);
            if (available_programs) free_program_array(available_programs);
            exit(1);
        }
        /* success: switch to batch mode and use 'batch' as input stream */
        infile = batch;
        is_interactive = 0;
    } else {
        is_interactive = 1;
    }

    while (1) { // Infinite loop to continuously prompt for input
        // --- INTERACTIVE MODE ---
        if(is_interactive) {
            printf("wish>");
            // Prompt user for input
            if (fgets(inputBuffer, sizeof(inputBuffer), stdin) != NULL) { // up to ((BUFFER_SIZE) - 1) chars + null terminator
                // Remove trailing newline, if it exists
                char *newline = strchr(inputBuffer, '\n');
                if (newline) {
                    *newline = '\0';
                } else {
                    // If no newline, input was too long and was truncated
                    printf("Input too long. Truncating.\n");
                    clear_stdin_buffer();
                }
                // Use new helpers for parsing and execution
                char *linecopy = strdup(inputBuffer);
                if (!linecopy) {
                    _shell_error_msg();
                    continue;
                }
                int part_count = 0;
                char **parts = split_parallel_commands(linecopy, &part_count);
                if (!parts) {
                    free(linecopy);
                    continue;
                }
                for (int i = 0; i < part_count; ++i) {
                    // Reject empty segment (syntax error)
                    if (!parts[i] || parts[i][0] == '\0') {
                        _shell_error_msg();
                        continue;
                    }
                    char *redir_target = NULL;
                    if (parse_redirection(parts[i], &redir_target) < 0) {
                        _shell_error_msg();
                        if (redir_target) free(redir_target);
                        continue;
                    }
                    // Tokenize the trimmed command
                    char cmdcopy[BUFFER_SIZE];
                    strncpy(cmdcopy, parts[i], BUFFER_SIZE - 1);
                    cmdcopy[BUFFER_SIZE - 1] = '\0';
                    char *tokens[BUFFER_SIZE / 2 + 1];
                    int token_count = tokenize_input(cmdcopy, tokens, BUFFER_SIZE / 2 + 1);
                    if (token_count == 0) {
                        if (redir_target) free(redir_target);
                        continue;
                    }
                    // Builtin check and redirection rejection
                    if (handle_builtin(tokens)) {
                        if (redir_target) {
                            _shell_error_msg();
                            free(redir_target);
                        }
                        continue;
                    }
                    // If no paths are set, print error and skip execution
                    if (shell_path_count == 0) {
                        _shell_error_msg();
                        if (redir_target) free(redir_target);
                        continue;
                    }
                    int executed = 0;
                    for (int j = 0; j < shell_path_count; j++) {
                        char fullpath[1024];
                        snprintf(fullpath, sizeof(fullpath), "%s/%s", shell_paths[j], tokens[0]);
                        if (access(fullpath, X_OK) == 0) {
                            pid_t pid = fork();
                            if (pid == 0) {
                                // Child: handle redirection
                                if (redir_target) {
                                    int fd = open(redir_target, O_CREAT | O_WRONLY | O_TRUNC, 0666);
                                    if (fd < 0) { _shell_error_msg(); _exit(1); }
                                    if (dup2(fd, STDOUT_FILENO) < 0) { _shell_error_msg(); _exit(1); }
                                    if (dup2(fd, STDERR_FILENO) < 0) { _shell_error_msg(); _exit(1); }
                                    close(fd);
                                }
                                execv(fullpath, tokens);
                                _shell_error_msg();
                                _exit(1);
                            } else {
                                wait(NULL);
                            }
                            executed = 1;
                            break;
                        }
                    }
                    if (!executed) {
                        _shell_error_msg();
                    }
                    if (redir_target) free(redir_target);
                }
                free(parts);
                free(linecopy);
                // End new helpers integration
            } else {
                // Handle EOF (Control+D) or input error
                if (feof(stdin)) {
                    // EOF encountered (Control+D pressed)
                    printf("\nGoodbye!\n");
                    break; // Exit the main loop gracefully
                } else {
                    // Handle other input errors
                    printf("Command not recognised, please try again.\n");
                }
            }
        } else {
            // --- BATCH MODE ---
            // Not implemented
            exit(1);
        }
        // --- INTERACTIVE MODE ---
        if(is_interactive) {
            printf("wish>");
            // Prompt user for input
            if (fgets(inputBuffer, sizeof(inputBuffer), stdin) != NULL) { // up to ((BUFFER_SIZE) - 1) chars + null terminator
                // Remove trailing newline, if it exists
                char *newline = strchr(inputBuffer, '\n');
                if (newline) {
                    *newline = '\0';
                } else {
                    // If no newline, input was too long and was truncated
                    printf("Input too long. Truncating.\n");
                    clear_stdin_buffer();
                }
                //printf("You entered: '%s'\n", inputBuffer);
                
                // Tokenize inputBuffer to extract command and arguments

                char *tokens[BUFFER_SIZE / 2 + 1];
                int token_count = tokenize_input(inputBuffer, tokens, BUFFER_SIZE / 2 + 1);

                // Skip empty input lines
                if (token_count == 0) {
                    continue;
                }

                // Use handle_builtin for all builtins
                if (handle_builtin(tokens)) {
                    continue; // builtin handled, do not fork
                }



        // If no paths are set, print error and skip execution
        if (shell_path_count == 0) {
            write(STDERR_FILENO, "An error has occurred\n", 22);
            continue;
        }
        int executed = 0;
        for (int i = 0; i < shell_path_count; i++) {
            char fullpath[1024];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", shell_paths[i], tokens[0]);
            if (access(fullpath, X_OK) == 0) {
                pid_t pid = fork();
                if (pid == 0) {
                    execv(fullpath, tokens);
                    // if execv returns, it failed
                    write(STDERR_FILENO, "An error has occurred\n", 21);
                    exit(1);
                } else {
                    wait(NULL);
                }
                executed = 1;
                break;
            }
        }
        if (!executed) {
            write(STDERR_FILENO, "An error has occurred\n", 22);
        }
            } else {
                // Handle EOF (Control+D) or input error
                if (feof(stdin)) {
                    // EOF encountered (Control+D pressed)
                    printf("\nGoodbye!\n");
                    break; // Exit the main loop gracefully
                } else {
                    // Handle other input errors
                    printf("Command not recognised, please try again.\n");
                }
            }
        } else {
            // --- BATCH MODE ---
            // Not implemented
            exit(1);
        }
    }
    
    // Cleanup before exit
    if (available_programs) {
        free_program_array(available_programs);
    }
    return 0;
}

/*
errno value | Error Description
------------|------------------
1 (EPERM)   | Operation not permitted
2 (ENOENT)  | No such file or directory
3 (ESRCH)   | No such process
4 (EINTR)   | Interrupted system call
5 (EIO)     | I/O error
6 (ENXIO)   | No such device or address
7 (E2BIG)   | Argument list too long
8 (ENOEXEC) | Exec format error
9 (EBADF)   | Bad file number
10 (ECHILD) | No child processes
11 (EAGAIN) | Try again
12 (ENOMEM) | Out of memory
13 (EACCES) | Permission denied
14 (EFAULT) | Bad address
15 (ENOTBLK)| Block device required
16 (EBUSY)  | Device or resource busy
17 (EEXIST) | File exists
18 (EXDEV)  | Cross-device link
19 (ENODEV) | No such device
20 (ENOTDIR)| Not a directory
*/
