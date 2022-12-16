/*
 * jshell.c
 * Simple shell program
 * This module contains the majority of the shell's core functionality
 * Author: Jaffar Alzeidi
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "command_parser.h"
#include "built-ins.h"

void interactive(struct built_in *b);
void batch(struct built_in *b, char *batch_file);

char *find_executable(char *name);
int run_command(struct program_data **pdata, size_t size, struct built_in *b);

int main(int argc, char **argv) {
    //Set up shell environment
    char shell_path[PATH_MAX];
    if(readlink("/proc/self/exe", shell_path, PATH_MAX) != -1) {
        setenv("shell", shell_path, 1);
    }
    setenv("PATH", "/bin", 1);

    //Prepare the built-in commands in an array
    struct built_in b[NUM_OF_BUILT_INS];
    store_builtins(b);

    //Call the appropriate shell mode
    if(argc == 1) interactive(b);
    else if(argc == 2) batch(b, argv[1]);
    else {
        printf("%s: invoked with invalid arguments\n", argv[0]);
        printf("Usage: %s or %s <batch_file>", argv[0], argv[0]);
    }
}

/*
 * Run shell in interactive mode
 * User is prompted to enter commands indefinitely until they exit shell
 */
void interactive(struct built_in *b) {
    char *prompt = "jshell> ";
    while(1) {
        //print prompt and read next line
        char cwd[PATH_MAX];
        if(getcwd(cwd, PATH_MAX) == NULL) {
             fprintf(stderr, "%s", "An error has occurred\n");
             continue;
        }
        printf("[%s]:%s", cwd, prompt);

        //get next line from keyboard
        char *line = NULL;
        size_t length = 0;
        errno = 0;
        ssize_t read = getline(&line, &length, stdin);
        if(read == -1) {
            free(line);
            if(errno == 0) {
                puts("");
                quit(0, NULL);
            } 
            else continue;
        }

        //parse and run command if parsing did not fail
        int size = 0;
        char **tokens = tokenize_command(line, &size);
        //if tokens[0] is null, the input was either empty or all whitespaces, either case is invalid
        if(tokens[0]) {
            struct program_data **pdata = NULL;
	        int last_index = 0;
            if(parse_command(&pdata, &last_index, tokens, size) != -1) {
                run_command(pdata, last_index + 1, b);
            }
	        for(int i = 0; i <= last_index; i++) free(pdata[i]);
	        free(pdata);
        }
        free(tokens);
        free(line);
    }
}

/*
 * Executes commands from a batch file
 */
void batch(struct built_in *b, char *batch_file) {
    FILE *f = fopen(batch_file, "r");
    if(!f) {
        fprintf(stderr, "%s", "An error has occurred\n");
        exit(1);
    }
    size_t length = 0;
    char *line = NULL;
    while(getline(&line, &length, f) != -1) {
        int size = 0;
        char **tokens = tokenize_command(line, &size);
        if(tokens[0]) {
            int last_index = 0;
            struct program_data **pdata = NULL;
            if(parse_command(&pdata, &last_index, tokens, size) == -1 || 
               run_command(pdata, last_index + 1, b) == -1) {
                fprintf(stderr, "%s", "An error has occurred\n");
                exit(1);
            }
	        for(int i = 0; i <= last_index; i++) free(pdata[i]);
	        free(pdata);
        }
        free(tokens);
    }
    free(line);
}

/*
 * Search the PATH for an executable whose name matches 'name'
 * Return full path of said executable, or NULL if not found
 */
char *find_executable(char *name) {
    char *path = getenv("PATH");
    if(path == NULL || (path = strdup(path)) == NULL) return NULL;
    char *token = strtok(path, ":");
    while(token != NULL) {
        size_t bytes = (strlen(token) + strlen(name) + 2) * sizeof(char); 
        char *executable_path = malloc(bytes);
        snprintf(executable_path, bytes, "%s/%s", token, name);
        if(access(executable_path, X_OK) == 0) {
            free(path); 
            return executable_path;
        }
        free(executable_path);
        token = strtok(NULL, ":");
    }
    free(path);
    return NULL;
}

/*
 * Finding a slash tells us that 'string' is a path to a program
 */
bool contains_slash(char *string) {
    char *temp = string;
    while(*temp != '\0') {
        if(*temp == '/') return true;
        ++temp;
    }
    return false;
}

/*
 * Wrapper for close(), sets *fd to -1 to indicate that *fd is not associated with a file
 */
void Close(int *fd) {
    if(*fd != -1) {
        close(*fd);
        *fd = -1;
    }
}

/*
 * Shell persists on failure, need to make sure that we don't keep too many file descriptors open.
 * Also, we need to free exec_path before losing the pointer, the last thing we want is a memory leak :)
 */
void free_resources(int *pipefd, int *stdin_cpy, int *stdout_cpy, char *exec_path) {
    Close(pipefd);
    Close(pipefd + 1);
    Close(stdin_cpy);
    Close(stdout_cpy);
    if(exec_path != NULL) {
        free(exec_path);
        exec_path = NULL;
    }
}

/*
 * If we have copies of stdio (stdin, stdout), then we use them to restore the originals and close copies
 */
void restore_io(int *stdin_cpy, int *stdout_cpy) {
    if(*stdin_cpy != -1) { 
        if(dup2(*stdin_cpy, 0) == -1) exit(1);
        Close(stdin_cpy);
    }
    if(*stdout_cpy != -1) {
        if(dup2(*stdout_cpy, 1) == -1) exit(1);
        Close(stdout_cpy);
    }
}

/*
 * Find full executable path (in case of non built-in) or index (in case of built-in) 
 */
void find_program(char *pname, char **exec_path, struct built_in *b, int *ibuilt_in) {
    if(contains_slash(pname)) {
        if(access(pname, X_OK) == 0) {
            *exec_path = malloc((strlen(pname) + 1) * sizeof(char));
            strcpy(*exec_path, pname);
        } else {
            perror("access");
        }
    } else {
        *ibuilt_in = find_builtin(pname, b);
        if(*ibuilt_in == -1) {
            *exec_path = find_executable(pname);
        }
    }
}

/*
 * Check if there is piping in the current program.
 * If the previous program is piped, it means that the current program's input will be the previous
 * program's output. 
 * If the current program is piped, then the next program's input is this program's output.
 * Return 0 on success, -1 on failure
 */
int check_piping(struct program_data **pdata, int i, int *pipefd) {
    if(i > 0 && pdata[i-1]->is_piped) {
        if(dup2(pipefd[0], 0) == -1) return -1;
        Close(pipefd);
    }
    if(pdata[i]->is_piped) {
        pipe(pipefd);
        if(dup2(pipefd[1], 1) == -1) return -1;
        Close(pipefd + 1);
    }
    return 0;
}

/*
 * Check for I/O redirection, mapping stdin and/or stdout accordingly
 * Return 0 on success, -1 on failure
 */
int check_redirection(struct program_data *pdata, int ibuilt_in) {
    if(pdata->output_file) {
        int output_fd;
        if(pdata->append_output) {
            output_fd = open(pdata->output_file, O_CREAT | O_WRONLY | O_APPEND, S_IRUSR | S_IWUSR);
        } else {
            output_fd = open(pdata->output_file, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
        }
        int status = dup2(output_fd, 1);
        close(output_fd);
        if(status == -1) return -1;
    }
    if(ibuilt_in == -1 && pdata->input_file) {
        int input_fd = open(pdata->input_file, O_RDONLY);
        int status = dup2(input_fd, 0);
        close(input_fd);
        if(status == -1) return -1;
    }
    return 0;
}

/*
 * If previous program piped or current program has an input_file, we make a copy of stdin because we
 * need to remap it, execute program, then restore it
 * If current program is piped or has an output_file, we make a copy of stdout for the same reason above
 */
int save_stdio(struct program_data **pdata, int i, int *stdin_cpy, int *stdout_cpy) {
    if((i > 0 && pdata[i-1]->is_piped) || pdata[i]->input_file) {
        if((*stdin_cpy = dup(0)) == -1) return -1;
    }
    if(pdata[i]->is_piped || pdata[i]->output_file) {
        if((*stdout_cpy = dup(1)) == -1) return -1;
    }
    return 0;
}

/*
 * Child code after fork
 */
void on_fork_child(struct program_data *p, int *pipefd, struct built_in *b, int ibuilt_in, char *exec_path) {
    char *shell_path = getenv("shell");
    if(shell_path) {
        setenv("parent", shell_path, 1);
    }
    if(p->is_piped) close(pipefd[0]);
    if(ibuilt_in != -1) {
        b[ibuilt_in].func(p->argc, p->argv);
        exit(0);
    } 
    else execv(exec_path, p->argv);
}

/*
 * Parent code after fork
 */ 
void on_fork_parent(bool is_daemon, char *exec_path, int pid) {
    if(exec_path) free(exec_path);
    if(!is_daemon) waitpid(pid, NULL, 0);
}

void on_fork_error(int *pipefd, int *stdin_cpy, int *stdout_cpy, char *exec_path) {
    fprintf(stderr, "%s", "An error has occurred\n");
    restore_io(stdin_cpy, stdout_cpy);
    free_resources(pipefd, stdin_cpy, stdout_cpy, exec_path);
}

/*
 * Run the command using the information from the array 'pdata'
 * If the program to run is an executable, we fork, then exec into its code
 * If it is a built-in, we only fork if the program must run in the background or has piping (to close
 * unused pipe end), then we call the built-in function
 * Otherwise, we just call the built-in function in the shell's process
 * Return 0 on success, -1 on failure
 */
int run_command(struct program_data **pdata, size_t size, struct built_in *b) {
    int pipefd[] = {-1, -1};
    for(int i = 0; i < size; i++) {
        int stdin_cpy = -1;
        int stdout_cpy = -1;

        char *exec_path = NULL;     //path of executable to run 
        int ibuilt_in = -1;         //index of built-in in 'b'
        find_program(pdata[i]->argv[0], &exec_path, b, &ibuilt_in);

        if((exec_path == NULL && ibuilt_in == -1) ||
           save_stdio(pdata, i, &stdin_cpy, &stdout_cpy) == -1 || 
           check_piping(pdata, i, pipefd) == -1 || 
           check_redirection(pdata[i], ibuilt_in) == -1) {

            restore_io(&stdin_cpy, &stdout_cpy);
            free_resources(pipefd, &stdin_cpy, &stdout_cpy, exec_path);
            fprintf(stderr, "%s", "An error has occurred\n");
            return -1;
        }

        if(ibuilt_in == -1 || pdata[i]->is_daemon || pdata[i]->is_piped ||
          (i > 0 && pdata[i-1]->is_piped)) {
            int pid = fork();
            if(pid > 0) on_fork_parent(pdata[i]->is_daemon, exec_path, pid);
            else if(pid == 0) on_fork_child(pdata[i], pipefd, b, ibuilt_in, exec_path);
            else {
                on_fork_error(pipefd, &stdin_cpy, &stdout_cpy, exec_path);
                return -1;
            }
        } 
        else {
            b[ibuilt_in].func(pdata[i]->argc, pdata[i]->argv);
        }
        restore_io(&stdin_cpy, &stdout_cpy);
        waitpid(-1, NULL, WNOHANG);     //check for and harvest zombie processes
    }
    return 0;
}
