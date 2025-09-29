#include <stdio.h>
#include <errno.h> // Error handling library. Assigns errno variable with error code when they occur.
#include <string.h> // For strerror()

int BUFFER_SIZE = 100; // Size of the input buffer

// Prints the current errno value and its description
// Use this function to throw an explained error without breaking out of the loop
void print_errno(void) {
    printf("Error Code: %s (%d)\n", strerror(errno), errno);
}

int main(int argc, char *argv[]) {
    char inputBuffer[BUFFER_SIZE]; // Buffer to hold user input
    int exit = 0; // Flag to control the loop

    while (!exit) { // Infinite loop to continuously prompt for input
        // Prompt user for input
        printf("wish>");
        if (fgets(inputBuffer, sizeof(inputBuffer), stdin) != NULL) { // up to bugg chars + null terminator
            printf("You entered: %s", inputBuffer);
        } else {
            // Handle error
            printf("Command not recognised, please try again.\n");
            if(strerror(errno) != NULL) // Only print errno if it's set
                print_errno();
        }
        inputBuffer[0] = '\0'; // Clear buffer by setting first character to null terminator
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