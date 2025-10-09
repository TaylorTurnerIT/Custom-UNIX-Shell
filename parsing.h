#ifndef PARSING_H
#define PARSING_H

// Parses the input line into command segments (parallel commands)
char **parse_input_line(char *line, int *num_segments);

// Parses a single command into tokens and extracts redirection target
int parse_command_segment(char *command, char **tokens, int max_tokens, char **redir_target);

// Tokenizes the entire input buffer (non-parallel mode)
int tokenize_input(char *input, char **tokens, int max_tokens);

#endif // PARSING_H
