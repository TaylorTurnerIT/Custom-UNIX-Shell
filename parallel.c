#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include "parallel.h"

//The one and only error message.
char error_msg[30] = "An error has occurred.\n";

void run_parallel_cmds(char* cmds[]) {

    //Keep doing this until we're fresh out of commands.
    int n = 0;
    while (cmds[n] != NULL) n++;

    //Store child process IDs here.
    pid_t pids[n];

    for (int i = 0; i < n; i++) {
	//Check for empty commands. Terminate the whole thing if so.
        if (cmds[i] == NULL || strlen(cmds[i]) == 0) {
            write(STDERR_FILENO, error_msg, strlen(error_msg));
            return;
        }
	//New process.
        pids[i] = fork();

	//Well, that's one failure of a fork if I say so myself.
        if (pids[i] < 0) {
            write(STDERR_FILENO, error_msg, strlen(error_msg));
        } else if (pids[i] == 0) {
            //Child process
            char* args[10]; //Command limit is 10 so you can't crash the computer. :)
            int argc = 0;

            char* token = strtok(cmds[i], " ");
            //Find spaces within commands.
            while (token != NULL && argc < 9) {
                args[argc++] = token;
                token = strtok(NULL, " ");
            }
	    //Set to null so execv works well.
            args[argc] = NULL;

            //Try /bin/ first
            char path[256];

	    //Time to execute!
            snprintf(path, sizeof(path), "/bin/%s", args[0]);
            execv(path, args);

            //If execv fails...
            write(STDERR_FILENO, error_msg, strlen(error_msg));
            exit(1);
        } else {
            // Debug Start
            #ifdef DEBUG
            printf("[parallel] started child PID %d for command: %s\n", pids[i], cmds[i]);
            fflush(stdout);
            #endif
        }
    }

    //Parent waits for all children
    for (int i = 0; i < n; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        // Debug Finish
        #ifdef DEBUG
        printf("[parallel] child PID %d finished (status=%d)\n", pids[i], status);
        fflush(stdout);
        #endif
    }
}