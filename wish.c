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




#include "wish.h"
#include "program_array.h"
#include "utils.h"
#include "command.h"



// Shell's search path (dynamic)
char **shell_paths = NULL;
int shell_path_count = 0;

#define BUFFER_SIZE 512


// Splits a line into tokens separated by spaces/tabs.
// Returns the number of tokens parsed.
int tokenize_input(char *input, char **tokens, int max_tokens) {
    int token_count = 0;
    char *token;

    while ((token = strsep(&input, " \t")) != NULL) {
        // Skip empty tokens from consecutive spaces/tabs
        if (*token == '\0') continue;
        if (token_count < max_tokens - 1) {
            tokens[token_count++] = token;
        }
    }
    tokens[token_count] = NULL;
    return token_count;
}

// BUFFER_SIZE is defined as a macro at the top

// Prints detailed errno information to stderr
// Use this function to throw an explained error without breaking out of the loop
void print_errno(void) {
    char error_msg[256];
    int len = snprintf(error_msg, sizeof(error_msg), 
                      "An error has occurred. %s (Code: %d)\n", 
                      strerror(errno), errno);
    if (len > 0 && len < (int)sizeof(error_msg)) {
        write(STDERR_FILENO, error_msg, len);
    } else {
        // Fallback if message is too long
        write(STDERR_FILENO, "An error has occurred\n", 22);
    }
}

// Helper to set errno and print error
void shell_error(int err_code) {
    errno = err_code;
    print_errno();
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
        execv(program, argv);
        // If execv returns, an error occurred
        print_errno();
        exit(1);
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
    }
}



/* ----------------- Helper Functions ----------------- */







int main(int argc, char *argv[]) {
    char *line = NULL; // Buffer for getline
    size_t len = 0; // Buffer length for getline
    ssize_t read; // Return value from getline
    int is_interactive = 1; // Flag for interactive mode
    FILE *infile = stdin; // Input stream (stdin for interactive, batch file for batch mode)

    /* Argument validation per spec:
     * - If more than one command-line argument is provided, print the single error
     *   message to stderr and exit(1).
     * - If one argument is provided, try to open it as batch file now.
     */
    if (argc > 2) {
        shell_error(E2BIG);
        exit(1);
    }
    if (argc == 2) {
        FILE *batch = fopen(argv[1], "r");
        if (!batch) {
            shell_error(ENOENT);
            exit(1);
        }
        infile = batch;
        is_interactive = 0;
    } else {
        is_interactive = 1;
    }

    ProgramArray *available_programs = get_all_programs();

    // Initialize shell path with default: /bin
    shell_path_count = 1;
    shell_paths = malloc(sizeof(char*));
    shell_paths[0] = strdup("/bin");

    // Note: Debug output removed per rubric requirements

    // Print initial prompt in interactive mode
    if (is_interactive) {
        printf("wish> ");
    }

    while (1) {
        // Read input from appropriate source using getline
        read = getline(&line, &len, infile);
        if (read == -1) {
            // Handle EOF or read error
            if (feof(infile)) {
                // EOF reached - exit gracefully as per rubric
                exit(0);
            } else {
                // Read error
                shell_error(EIO);
                exit(1);
            }
        }

        // Remove trailing newline
        if (read > 0 && line[read-1] == '\n') {
            line[read-1] = '\0';
        }

        // Skip empty lines
        char *trimmed = trim_whitespace(line);
        if (*trimmed == '\0') {
            if (is_interactive) {
                printf("wish> ");
            }
            continue;
        }

        // Process the command line (handles parallel commands, redirection, etc.)
        process_command_line(line);
        
        // Print prompt for next iteration in interactive mode
        if (is_interactive) {
            printf("wish> ");
        }
    }
    
    // Cleanup before exit
    if (line) {
        free(line);
    }
    if (infile != stdin) {
        fclose(infile);
    }
    
    // Free shell paths
    for (int i = 0; i < shell_path_count; i++) {
        free(shell_paths[i]);
    }
    free(shell_paths);
    
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
