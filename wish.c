#include <stdio.h>
#include <errno.h> // Error handling library. Assigns errno variable with error code when they occur.

int main(int argc, char *argv[]) {
    char inputBuffer[100];
    int exit = 0;
    while (!exit) { // Infinite loop to continuously prompt for input
        // Prompt user for input
        printf("wish>");
        if (fgets(inputBuffer, sizeof(inputBuffer), stdin) != NULL) { // up to 99 chars + null terminator
            printf("You entered: %s", inputBuffer);
        } else {
            // Handle error
            printf("Command not recognised, please try again.\n");
            printf("DEBUG: Value of errno: %d\n", errno); 
        }
        inputBuffer[0] = '\0'; // Clear buffer by setting first character to null terminator
    }
    return 0;
}