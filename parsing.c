#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parsing.h"

// Splits the input line into separate command segments (for parallel commands)
char **parse_input_line(char *line, int *num_segments) {
    char **segments = malloc(sizeof(char *));
    segments[0] = strdup(line);
    *num_segments = 1;
    return segments;
}

// Splits a single command into tokens and detects redirection
int parse_command_segment(char *command, char **tokens, int max_tokens, char **redir_target) {
    int count = 0;
    char *token = strtok(command, " \t\n");
    while (token != NULL && count < max_tokens) {
        tokens[count++] = token;
        token = strtok(NULL, " \t\n");
    }
    *redir_target = NULL; // no redirection handled yet
    return count;
}

// Tokenizes the entire input buffer (for non-parallel mode)
int tokenize_input(char *input, char **tokens, int max_tokens) {
    return parse_command_segment(input, tokens, max_tokens, NULL);
}
