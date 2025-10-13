#include "program_array.h"
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

int is_executable(const char *filepath) {
    struct stat st;
    if (stat(filepath, &st) == 0) {
        return (st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH);
    }
    return 0;
}

int add_program(ProgramArray *arr, const char *program) {
    if (arr->count >= arr->capacity) {
        arr->capacity *= 2;
        char **new_programs = realloc(arr->programs, arr->capacity * sizeof(char*));
        if (!new_programs) {
            errno = ENOMEM;
            return -1;
        }
        arr->programs = new_programs;
    }
    arr->programs[arr->count] = malloc(strlen(program) + 1);
    if (!arr->programs[arr->count]) {
        errno = ENOMEM;
        return -1;
    }
    strcpy(arr->programs[arr->count], program);
    arr->count++;
    return 0;
}

int scan_bin_directory(ProgramArray *arr, const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        return -1;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        size_t path_len = strlen(dir_path) + 1 + strlen(entry->d_name) + 1;
        char *filepath = malloc(path_len);
        if (!filepath) {
            closedir(dir);
            errno = ENOMEM;
            return -1;
        }
        int written = snprintf(filepath, path_len, "%s/%s", dir_path, entry->d_name);
        if (written < 0 || (size_t)written >= path_len) {
            free(filepath);
            continue;
        }
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

ProgramArray* get_all_programs() {
    ProgramArray *programs = malloc(sizeof(ProgramArray));
    if (!programs) {
        errno = ENOMEM;
        return NULL;
    }
    programs->capacity = 100;
    programs->count = 0;
    programs->programs = malloc(programs->capacity * sizeof(char*));
    if (!programs->programs) {
        free(programs);
        errno = ENOMEM;
        return NULL;
    }
    const char *bin_dirs[] = {"/bin", "/usr/bin", "/usr/local/bin", "/sbin", "/usr/sbin", NULL};
    for (int i = 0; bin_dirs[i] != NULL; i++) {
        scan_bin_directory(programs, bin_dirs[i]);
    }
    return programs;
}

void free_program_array(ProgramArray *arr) {
    if (arr) {
        for (int i = 0; i < arr->count; i++) {
            free(arr->programs[i]);
        }
        free(arr->programs);
        free(arr);
    }
}
