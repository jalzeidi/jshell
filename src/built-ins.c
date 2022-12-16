/*
 * built-ins.c
 * This module contains implementation of built-in shell functions for the program 'jshell'
 * Author: Jaffar Alzeidi
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <sys/wait.h>
#include <libgen.h>
#include "built-ins.h"

/*
 * Change the current working directory
 */
void cd(int argc, char **argv) {
    if(argc == 2) {
        char cwd[PATH_MAX];
        if(chdir(argv[1]) == -1 || getcwd(cwd, PATH_MAX) == NULL || setenv("PWD", cwd, 1) == -1) {
            fprintf(stderr, "%s", "An error has occurred\n");
        }
    } else if(argc > 2) {
        fprintf(stderr, "%s", "Invalid args\n");
    }
}

/*
 * Clear the screen
 */
void clr(int argc, char **argv) {
    printf("%s", "\033[H\033[2J");
}

/*
 * List contents of current or specified directory
 */
void dir(int argc, char **argv) {
    DIR *d = NULL;
    if(argc == 1) {
        char cwd[PATH_MAX];
        if(getcwd(cwd, PATH_MAX) == NULL) {
            fprintf(stdout, "%s", "An error has occurred\n");
            return;
        }
        d = opendir(cwd);
    } else if(argc == 2) {
        d = opendir(argv[1]);
    }
    if(d == NULL) {
        fprintf(stderr, "%s: invalid directory\n", argv[1]);
        return;
    }
    struct dirent *entry = NULL;
    while((entry = readdir(d)) != NULL) {
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        printf("%s\n", entry->d_name);
    }
    closedir(d);
}

/*
 * Print every environment variable
 */
void show_environ(int argc, char **argv) {
    extern char **environ;
    char **temp = environ;
    while(*temp != NULL) {
        printf("%s\n", *temp);
        ++temp;
    }
}

/*
 * Set the PATH environment variable.
 * The argument is a colon-separated string, where each substring is a directory path
 */
void set_path(int argc, char **argv) {
    if(argc == 1) setenv("PATH", "", 1);
    else if(argc == 2) {
        size_t length = strlen(argv[1]) + 1;
        for(int i = 2; i < argc; i++) length += strlen(argv[i]) + 1;
        char path[length];
        strcpy(path, argv[1]);
        for(int i = 2; i < argc; i++) {
            strcat(path, ":");
            strcat(path, argv[i]);
        }
        setenv("PATH", path, 1);
    } else {
        fprintf(stderr, "%s", "An error has occurred\n");
    }
}

/*
 * Used to print a replication of provided input and/or display environment variables
 */
void echo(int argc, char **argv) {
    for(int i = 1; i < argc; i++) {
        if((strlen(argv[i]) > 1) && argv[i][0] == '$') {
            const char *value;
            if((value = getenv(argv[i] + 1)) != NULL) printf("%s ", value);
        } 
        else printf("%s ", argv[i]);
    }
    puts("");
}

/*
 * Display the user manual using the 'more' program
 * User manual is assumed to be in the same directory as the shell's executable
 * The shell executable path is in the environment, we get it, trim the filename of the shell out of it
 * to get the directory.
 * We then use the directory and append the user manual's filename to get its full path
 */
void help(int argc, char **argv) {
    const char *const README_NAME = "readme_doc";

    char *shell_path = getenv("shell");
    if(!shell_path) {
        fprintf(stderr, "%s", "An error has occurred\n");
        return;
    }
    int length = strlen(shell_path);
    char shell_path_cpy[length + 1];
    strcpy(shell_path_cpy, shell_path);
    const char *shell_root = dirname(shell_path_cpy);   //we use a copy because dirname can modify input

    length = strlen(shell_root) + 1 + strlen(README_NAME) + 1;
    char readme_path[length];
    snprintf(readme_path, length, "%s/%s", shell_root, README_NAME);
    fprintf(stderr, "readme_path: %s\n", readme_path);


    int pid = fork();
    if(pid > 0) {
        waitpid(pid, NULL, 0);
    } else if(pid == 0) {
        char *const argv[] = {"more", readme_path, NULL};
        execv("/bin/more", argv);
    } else {
        fprintf(stderr, "%s", "An error has occurred\n");
    }
}

/*
 * Pause shell execution until enter is pressed
 */
void pause_shell(int argc, char **argv) {
    while(getchar() != '\n');
}

void quit(int argc, char **argv) {
    exit(0);
}

/*
 * Searches array of built-ins for a built-in that matches the command name
 */
int find_builtin(char *command, struct built_in *b) {
    for(int i = 0; i < NUM_OF_BUILT_INS; i++) {
        if(strcmp(b[i].name, command) == 0) return i;
    }
    return -1;
}

void store_builtins(struct built_in *b) {
    b[0].name = "cd";
    b[0].func = cd;

    b[1].name = "clr";
    b[1].func = clr;

    b[2].name = "dir";
    b[2].func = dir;

    b[3].name = "environ";
    b[3].func = show_environ;

    b[4].name = "path";
    b[4].func = set_path;

    b[5].name = "echo";
    b[5].func = echo;

    b[6].name = "help";
    b[6].func = help;

    b[7].name = "pause";
    b[7].func = pause_shell;

    b[8].name = "quit";
    b[8].func = quit;
}
