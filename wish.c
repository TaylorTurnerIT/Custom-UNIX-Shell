#include <stdio.h>
#include <errno.h> // Error handling library. Assigns errno variable with error code when they occur.
#include <string.h> // For strerror()

int BUFFER_SIZE = 500; // Size of the input buffer

// Clears the standard input buffer, effectively cin.ignore()
void clear_stdin_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// Prints the current errno value and its description
// Use this function to throw an explained error without breaking out of the loop
void print_errno(void) {
    printf("ERROR: %s (Code: %d)\n", strerror(errno), errno);
}

int main(int argc, char *argv[]) {
    char inputBuffer[BUFFER_SIZE]; // Buffer to hold user input
    int loop = 0; // Flag to control the loop

    if (argc > 2) {
        errno = 7; // E2BIG: Argument list too long
        print_errno();
        return 7;
    } else if (argc == 2) {
        // Batch mode not implemented
        printf("Batch mode not implemented.\n");
        return 1;
    }

    while (!loop) { // Infinite loop to continuously prompt for input
        // Prompt user for input
        printf("wish>");
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
            printf("You entered: '%s'\n", inputBuffer);
        } else {
            // Handle error
            printf("Command not recognised, please try again.\n");
            if(strerror(errno)) // Only print errno if it exists. By default the value is junk (not necessarily 0)
                print_errno();
        }
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