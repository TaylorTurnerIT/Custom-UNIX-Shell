#ifndef PROGRAM_ARRAY_H
#define PROGRAM_ARRAY_H

#include <stdio.h>

// Structure to hold the array of programs
typedef struct {
    char **programs;
    int count;
    int capacity;
} ProgramArray;

int is_executable(const char *filepath);
int add_program(ProgramArray *arr, const char *program);
int scan_bin_directory(ProgramArray *arr, const char *dir_path);
ProgramArray* get_all_programs();
void free_program_array(ProgramArray *arr);

#endif // PROGRAM_ARRAY_H
