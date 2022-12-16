/*
 * command_parser.h
 * This module does all of the parsing work for the shell program 'jshell'
 * An array of structs is produced as a result of parsing
 * Each struct in the array corresponds to a command (built-in or executable)
 * Each struct represents run configuration and data 
 * Author: Jaffar Alzeidi
 */

#include <stdbool.h>

//Holds data for the program (or shell built-in) to run
struct program_data {
    char **argv;            //the program's arguments, first arg is the program's name
    int argc;               //number of strings in argv
    char *input_file;       //the file to replace stdin while this program is running
    char *output_file;      //the file to replace stdout while this program is running
    bool append_output;     //if true, we append to output_file, otherwise, we overwrite it
    bool is_piped;          //does this program's output flow to another program's input?
    bool is_daemon;         //should we run this program in the background?
};

//tokenizes the whitespace delimited string 'command' into an array of strings
char **tokenize_command(char *command, int *next);

/* Uses the result of tokenize_command to extract the command's data
 * Data includes: programs to run, program arguments, piping, I/O redirection, background running
 * Extracted data is saved in an array of struct program_data, must be free'd
 * If parsing fails, -1 is returned, otherwise, 0 is returned
 */
int parse_command(struct program_data ***pdata, int *next, char **tokens, int size);