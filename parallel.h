#ifndef PARALLEL_H
#define PARALLEL_H

#include<stdbool.h>

#define arg_max 10

typedef struct {
     char* name;
     char* args[arg_max];
     int arg_count;
} Command;

void run_parallel_cmds(char* cmds[]);

#endif
