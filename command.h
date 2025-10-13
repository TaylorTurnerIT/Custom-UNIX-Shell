#ifndef COMMAND_H
#define COMMAND_H

// Command parsing and execution
int process_command_line(char *line);
int parse_redirection(char *cmd, char **out_target);
char **split_parallel_commands(char *linecopy, int *out_count);

#endif // COMMAND_H
