#ifndef WISH_H
#define WISH_H

#include <stdio.h>


// Shell's search path (dynamic)
extern char **shell_paths;
extern int shell_path_count;

// Error handling
void shell_error(int err_code);
void print_errno(void);

// Input parsing
int tokenize_input(char *input, char **tokens, int max_tokens);

#endif // WISH_H
