/*
 * built-ins.h
 * This module contains declarations of built-in shell functions for the program 'jshell'
 * Author: Jaffar Alzeidi
 */

#define NUM_OF_BUILT_INS 9

//stores the name of a command with a pointer to its function
//an array of this struct is used to easily look up valid built-in commands and call
//their corresponding functions
struct built_in {
    char *name;
    void (*func)(int, char **);
};

//built-in commands
void cd(int argc, char **argv);
void clr(int argc, char **argv);
void dir(int argc, char **argv);
void show_environ(int argc, char **argv);
void set_path(int argc, char **argv);
void echo(int argc, char **argv);
void help(int argc, char **argv);
void pause_shell(int argc, char **argv);
void quit(int argc, char **argv);

//utilities for finding and storing built-ins
int find_builtin(char *command, struct built_in *b);
void store_builtins(struct built_in *b);
