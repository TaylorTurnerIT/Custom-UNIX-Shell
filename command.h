#ifndef COMMAND_H
#define COMMAND_H

// Command parsing and execution
int process_command_line(char *line);
int parse_redirection(char *cmd, char **out_target);
char **split_parallel_commands(char *linecopy, int *out_count);

/* Builtin commands available in the shell
 * - exit: exit the shell (no arguments)
 * - cd <dir>: change working directory
 * - path [paths...]: set the executable search paths
 * - sleep <seconds>: sleep the shell for a non-negative integer number
 *   of seconds (implemented using nanosleep to avoid busy-waiting)
 */

#endif // COMMAND_H
