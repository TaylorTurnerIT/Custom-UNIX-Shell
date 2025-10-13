#ifndef WISH_H
#define WISH_H

#include <stdio.h>


// Shell's search path (dynamic)
extern char **shell_paths;
extern int shell_path_count;

// Error handling
void shell_error(int err_code);
void print_errno(void);

#endif // WISH_H
