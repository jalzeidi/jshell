/*
 * command_parser.c
 * Part of the 'jshell' project
 * Implementation of command_parser.h interface
 * Used to parse a shell command
 * Author: Jaffar Alzeidi
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "command_parser.h"
#include "vector.h"

void init_program_data(struct program_data *p, char **argv, int argc);

char **tokenize_command(char *command, int *next) {
    int capacity = 8;
    char **tokens = malloc(capacity * sizeof(char *));
    if(tokens == NULL) {
        perror("malloc");
	    exit(1);
    }
    char *token = strtok(command, " \t\n");
    while(token != NULL) {
        tokens = check_vector(tokens, &capacity, *next);
        tokens[*next] = token;
        ++*next;
        token = strtok(NULL, " \t\n");
    }
    tokens = check_vector(tokens, &capacity, *next);
    tokens[*next] = NULL;
    ++*next;
    return tokens;
}

/*
 * Takes an array of strings 'tokens', and parses it to produce an array of program_data structs (pdata) 
 * The word 'operator' will be used to refer to the following: '>', '>>', '<', '|', '&'
 * This function loops over every string in tokens, looking for operators
 * The first operator to be found marks the end of the argv array for the current element of pdata
 * Every time a '|' is encountered there is a new program, because pipe by definition chains programs
 * In the case that a '&' is found and it is NOT the last string in 'tokens', there is a new program
 * In case of a new program, pdata is reallocated, and a new struct is allocated for the program
 */
int parse_command(struct program_data ***pdata, int *next, char **tokens, int size) {
    size_t arg_start = 0;       //the start index of the current program's arguments
    bool found_args = false;    //have we located the list of arguments (marked its end)?

    int capacity = 1;
    *pdata = malloc(capacity * sizeof(struct program_data *));

    (*pdata)[0] = malloc(sizeof(struct program_data));
    init_program_data((*pdata)[0], tokens, size - 1);

    for(int i = 0; i < size - 1; i++) {
        int redir_in = strcmp(tokens[i], "<") == 0; 
        int redir_out = strcmp(tokens[i], ">") == 0;
        int redir_out_append = strcmp(tokens[i], ">>") == 0;
        int do_pipe = strcmp(tokens[i], "|") == 0;
        int run_daemon = strcmp(tokens[i], "&") == 0;

        if(redir_in || redir_out || redir_out_append || do_pipe ||  run_daemon) {
            if(i - 1 < arg_start || (!run_daemon && (i + 1 >= size - 1))) {
                fprintf(stderr, "%s", "An error has occurred\n");
		        return -1;   
	        }

            if(!found_args) {
	            tokens[i] = NULL;
		        found_args = true;
		        (*pdata)[*next]->argc = i - arg_start;
	        }

            if(redir_in) {
	            (*pdata)[*next]->input_file = tokens[i + 1];
		        i++;
	        } else if(redir_out || redir_out_append) {
	            (*pdata)[*next]->output_file = tokens[i + 1];
		        i++;
		        if(redir_out_append) (*pdata)[*next]->append_output = true;
	        } else if(do_pipe || run_daemon) {
                if(do_pipe) (*pdata)[*next]->is_piped = true;
                else (*pdata)[*next]->is_daemon = true;
                if(do_pipe || i + 1 < size - 1) {
                    arg_start = i + 1;
		            found_args = false;
		            ++*next;
 	                *pdata = check_vector(*pdata, &capacity, *next);
		            (*pdata)[*next] = malloc(sizeof(struct program_data));
		            init_program_data((*pdata)[*next], tokens + i + 1, size - 1 - arg_start);
                }
            }
	    }
    }
    return 0;
}

void init_program_data(struct program_data *p, char **argv, int argc) {
    p->argv = argv;
    p->argc = argc;
    p->input_file = NULL;
    p->output_file = NULL;
    p->append_output = false;
    p->is_piped = false;
    p->is_daemon = false;
}
