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

int BUFFER_SIZE = 4096; // Size of the input buffer

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
        char **new_programs = realloc(arr->programs, arr->capacity * sizeof(char*));
        if (!new_programs) {
            errno = ENOMEM;
            return -1;
        }
        arr->programs = new_programs;
    }
    
    // Allocate memory for the program name
    arr->programs[arr->count] = malloc(strlen(program) + 1);
    if (!arr->programs[arr->count]) {
        errno = ENOMEM;
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
    
    ProgramArray *available_programs = get_all_programs();

    if (available_programs) {
        printf("Found %d programs in system bin directories.\n", available_programs->count);
    } else {
        printf("Failed to load programs from bin directories.\n");
        print_errno();
    }

    // Argument validation
    if (argc > 2) {
        printf("An error has occurred\nArgument list too long (Code: %d)\n", E2BIG);
        if (available_programs) free_program_array(available_programs);
        exit(E2BIG);
    } else if (argc == 2) {
        // Batch mode not implemented
        printf("Batch mode not implemented.\n");
        is_interactive = 0; // Switch to batch mode
        if (available_programs) free_program_array(available_programs);
        exit(1);
    } else {
        is_interactive = 1; // Interactive mode
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
                //printf("You entered: '%s'\n", inputBuffer);
                
                // Tokenize inputBuffer to extract command and arguments
                char *tokens[BUFFER_SIZE / 2 + 1]; // Max possible tokens
                int token_count = 0;
                char *saveptr;
                char *token = strtok_r(inputBuffer, " \t", &saveptr);
                while (token && token_count < (BUFFER_SIZE / 2)) {
                    tokens[token_count++] = token;
                    token = strtok_r(NULL, " \t", &saveptr);
                }
                tokens[token_count] = NULL;

                int found = 0;
                if (available_programs && token_count > 0) {
                    for (int i = 0; i < available_programs->count; i++) {
                        if (strcmp(tokens[0], available_programs->programs[i]) == 0) {
                            found = 1;
                            fork_and_run(tokens[0], tokens);
                            break;
                        }
                    }
                }
                if (!found) {
                    printf("Command not recognised, please try again.\n");
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