//:
// Created by Tom Paulus on 2/22/18.
// CS 570 -- Carroll
// Due: 4/20/2018 11 PM for EC
//      4/25/2018 11 PM for Regular Credit
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedImportStatement"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include "p2.h"
#include "getword.h"

#define MAX_PIPES 10

#define TRUE 1
#define FALSE 0

//#define DEBUG  // Uncomment to print Debug Statements

// Constants
const int FOUT_FLAGS = O_CREAT | O_EXCL | O_APPEND | O_WRONLY;
const int FIN_FLAGS = O_RDONLY;

// Line FLags
int FLAG_BAD_LINE = FALSE;
int FLAG_SCRIPT_MODE = FALSE;   // If a Script File is providing the lines; suppresses prompts
int FLAG_EOF = FALSE;           // EOF
int FLAG_COMMENT = FALSE;       // #
int FLAG_DETACH = FALSE;        // &
int FLAG_PIPE = FALSE;          // |
int FLAG_OUT_REDIR = FALSE;     // >
int FLAG_IN_REDIR = FALSE;      // <

// Globals
char *input_file = NULL;
char *output_file = NULL;

char user_input[STORAGE];
char *parsed_line[MAXITEM];
char **line_ptr = parsed_line;

char **arg_sets[MAX_PIPES + 1];
int pipe_types[MAX_PIPES];
char *pipe_locations[MAX_PIPES];
int num_pipes;

int main(int argc, char **argv) {
    int line_length, i, arg_length, arg_file;
    char **arg_set;
    char **beginning;

    setpgid(0, 0);
    (void) signal(SIGTERM, signal_handler);

    // Argv:  File Name; Script File
    if (argc > 1) {
        if ((arg_file = open(argv[1], FIN_FLAGS)) == -1) {
            fprintf(stderr, "Failed to open script. Double-check your arguments!\n");
            exit(1);
        } else {
            dup2(arg_file, STDIN_FILENO);
            close(arg_file);
            FLAG_SCRIPT_MODE = TRUE;
        }
    }

    for (;;) {
        init();
        line_length = parse(parsed_line); // Let input from command line

        if (FLAG_EOF) break;  // EOF received from input

        if (line_length == 0) {
            if (FLAG_OUT_REDIR || FLAG_IN_REDIR || FLAG_PIPE || FLAG_DETACH) {
                fprintf(stderr, "No executable command - check entry\n");
                continue;
            } else {
                continue;  // Empty Line
            }
        }

        if (line_length == -1) continue; // Bad Input Line - From Parse

        if ((FLAG_OUT_REDIR && (output_file == NULL)) ||
            (FLAG_IN_REDIR && (input_file == NULL))) {
            fprintf(stderr, "Invalid Redirection Format\n");
            continue;
        }

        if (builtin_handler(parsed_line, line_length) == 1) continue;
        else if (!FLAG_PIPE) exec_simple(parsed_line, line_length);
        else {
            // Split the args array into the part before the pipe and the part after the pipe

            beginning = line_ptr;
            arg_length = 0;
            i = 0;
            while (i <= num_pipes) {   // Number of Pipes + 1 for last command
                if ((*line_ptr >= pipe_locations[i] && pipe_locations[i] != NULL) || *line_ptr == NULL) {
                    // New Set of Arguments
                    arg_set = (char **) malloc(sizeof(char **) * MAXITEM);
                    memcpy(arg_set, beginning, sizeof(char **) * (line_ptr - beginning));
                    arg_sets[i] = arg_set;

                    // Prep for next set
                    beginning = line_ptr;
                    i++;
                    arg_length = 0;
                } else {
                    line_ptr++;
                    arg_length++;
                }
            }

            if (num_pipes > 0) exec_piped(arg_sets, pipe_types, num_pipes);  // fixme

            for (i = 0; i < num_pipes; i++) {
                // Free Malloc-ed Space Above
                free(arg_sets[i]);
            }
        }
    }

    killpg(getpgrp(), SIGTERM);
    if (!FLAG_SCRIPT_MODE) printf("p2 terminated.\n");

    _exit(0);
}

/**
 * Print Prompt, Reset Flags and Clear Argument Arrays
 */
void init() {
    if (!FLAG_SCRIPT_MODE) printf("p2: ");  // Suppress Prompt for Script Mode

    // Reset Args Array
    memset(&parsed_line, 0, sizeof parsed_line);
    memset(&arg_sets, 0, sizeof arg_sets);
    memset(&pipe_types, 0, sizeof pipe_types);
    memset(&pipe_locations, 0, sizeof pipe_locations);

    // Reset Flags
    FLAG_BAD_LINE = FALSE;
    FLAG_EOF = FALSE;
    FLAG_COMMENT = FALSE;
    FLAG_DETACH = FALSE;
    FLAG_PIPE = FALSE;
    FLAG_OUT_REDIR = FALSE;
    FLAG_IN_REDIR = FALSE;

    // Reset Pipes
    input_file = NULL;
    output_file = NULL;
    line_ptr = parsed_line;
    num_pipes = 0;
}

/**
 * Take in (from Standard In) a line and parse it.
 * Input is stored in the "user_input" array, and the parsed arguments are returned as pointers to strings in the
 * "user_input" array.
 */
int parse(char **args) {
    int word_length, pipe_type;
    char *s_prt = user_input;
    int input_length = 0; // Number of words entered by the user

    word_length = getword(s_prt); // Get first word
    while (word_length != 0 && word_length != -10) {
        if (FLAG_SCRIPT_MODE && FLAG_COMMENT) {
            // Skip Words after Comment Mark when reading in a file
            word_length = getword(s_prt);
            continue;
        }
        if (word_length < 0) {
            // Meta Characters
            if (*s_prt == '#') {
                if (input_length == 0) {
                    FLAG_COMMENT = TRUE;
                } else {
                    if (FLAG_SCRIPT_MODE) FLAG_COMMENT = TRUE;
                    s_prt += abs(word_length + 1);
                    word_length = getword(s_prt);
                    continue;
                }
            } else if (*s_prt == '>') {
                // Next Argument will be the Destination File
                if (FLAG_OUT_REDIR) {
                    fprintf(stderr, "Output file already defined\n");
                    FLAG_BAD_LINE = TRUE;
                }

                FLAG_OUT_REDIR = TRUE;
                word_length = getword(s_prt);
                continue;
            } else if (*s_prt == '<') {
                if (FLAG_IN_REDIR) {
                    fprintf(stderr, "Input file already defined\n");
                    FLAG_BAD_LINE = TRUE;
                }

                // Next Argument will be the Input File
                FLAG_IN_REDIR = TRUE;
                word_length = getword(s_prt);
                continue;
            } else if (*s_prt == '|' || strcmp(s_prt, "|&") == 0) {
                FLAG_PIPE = TRUE;

                pipe_type = (strcmp(*line_ptr, "|&") == 0) ? 1 : 0; // Pipe Type 1 is a '|&' pipe

                // Save pipe location
                pipe_types[num_pipes] = pipe_type;
                pipe_locations[num_pipes++] = s_prt;

                s_prt += abs(word_length + 1);
                word_length = getword(s_prt);
                continue;
            } else if (*s_prt == '&') {
                FLAG_DETACH = TRUE;
                break;
            }
        }

        if (FLAG_OUT_REDIR && output_file == NULL &&
            FLAG_IN_REDIR && input_file == NULL) {
            fprintf(stderr, "Ambiguous IO routing descriptor - check format\n");
            return 0;
        }

        if (FLAG_OUT_REDIR && output_file == NULL) {
            output_file = s_prt;
            s_prt += abs(word_length + 1);
            word_length = getword(s_prt);
            continue;
        }

        if (FLAG_IN_REDIR && input_file == NULL) {
            input_file = s_prt;
            s_prt += abs(word_length + 1);
            word_length = getword(s_prt);
            continue;
        }

        // Parse Regular Arguments
        args[input_length++] = s_prt; // Save Input Word
        s_prt += abs(word_length + 1); // Move Input Buffer Pointer

        word_length = getword(s_prt); // Get next word
    }

    if (word_length == 0) FLAG_EOF = TRUE; // EOF Received
    if (FLAG_BAD_LINE) input_length = -1;

    return !FLAG_COMMENT || FLAG_SCRIPT_MODE ? input_length : 0; // Return Length 0 if comment line
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/**
 * Execute a Simple (non-piped) command
 */
void exec_simple(char **args, int arg_len) {
    int input_fd = -1;
    int output_fd = -1;
    pid_t child_pid;

#ifdef DEBUG
    int i;
    printf("%d arguments - ", arg_len);
    for (i = 0; i < sizeof(args); ++i) {
        printf("%s, ", args[i]);
    }
    printf("\n");
#endif

    if (FLAG_DETACH && !FLAG_IN_REDIR) {
        // Redirect Input to /dev/null for Detached Child
        FLAG_IN_REDIR = TRUE;
        input_file = "/dev/null";
    }


    if (FLAG_IN_REDIR) {
        if ((input_fd = open(input_file, FIN_FLAGS, S_IRUSR | S_IWUSR)) < 0) {
            perror("Could not set input redirection file");
            return;
        }
    }

    if (FLAG_OUT_REDIR) {
        if ((output_fd = open(output_file, FOUT_FLAGS, S_IRUSR | S_IWUSR)) < 0) {
            perror("Could not set output redirection file");
            return;
        }
    }

    // Forking a child
    fflush(stdout);
    child_pid = fork();


    if (child_pid == -1) {
        perror("\nFailed forking child.\n");
        return;
    } else if (child_pid == 0) {
        // This is run by the child process

        if (FLAG_IN_REDIR) dup2(input_fd, STDIN_FILENO);
        if (FLAG_OUT_REDIR) dup2(output_fd, STDOUT_FILENO);

        if (execvp(args[0], args) < 0) {
            perror("Could not execute command - Invalid Command");
            _exit(9);
        }
        _exit(0);
    } else if (!FLAG_DETACH) {
        // Wait for forked child process to complete if process is attached to parent
        pid_t completed_child = 0;
        while (completed_child != child_pid) {
            completed_child = wait(NULL);
        }
    } else {
        // Print PID for child task
        printf("%s [%d]\n", args[0], child_pid);
    }

    if (input_fd != -1 && close(input_fd) != 0) perror("Could not close Input File Descriptor");
    if (output_fd != -1 && close(output_fd) != 0) perror("Could not close Output File Descriptor");
}

#pragma clang diagnostic pop

/**
 * Execute a Piped Command, where the output of the first command is redirected to the input of the next.
 */
void exec_piped(char **arg_sets[], int pipe_types[], int num_pipes) {
    // 0 is read end, 1 is write end
    int pipefds[num_pipes][2];

    pid_t p_child;
    int input_fd;
    int output_fd;
    int i;

#ifdef DEBUG
    char **args1_ptr = arg_sets[0];
    char **args2_ptr = arg_sets[num_pipes];

    printf("Arg Set 1 - ");
    while (*args1_ptr != NULL) {
        printf("%s; ", *args1_ptr);
        args1_ptr++;
    }

    printf("\nArg Set 2 - ");
    while (*args2_ptr != NULL) {
        printf("%s; ", *args2_ptr);
        args2_ptr++;
    }
    printf("\n");
#endif

    // Fork Child 1
    fflush(stdout);
    fflush(stderr);
    p_child = fork();
    if (p_child < 0) {
        printf("\nCould not fork");
        return;
    }

#ifdef DEBUG
    if (p_child != 0) {
        printf("Child 1 PID - %d\n", p_child);
    } else {
        printf("I am Pipe Child 1!\n");
    }
#endif

    if (p_child == 0) {
        // Child 1 executing...

        // Make Pipe
        if (pipe(pipefds[0]) < 0) {
            perror("\nPipe could not be initialized");
            return;
        }

        // Fork Child 2
        fflush(stdout);
        fflush(stderr);
        p_child = fork();

        if (p_child < 0) {
            printf("\nCould not fork");
            return;
        }

#ifdef DEBUG
        if (p_child != 0) {
            printf("Child 2 PID - %d\n", p_child);
        } else {
            printf("I am Pipe Child 2!\n");
        }
#endif

        // Child 2 executing
        if (p_child == 0) {
            for (i = 1; i < num_pipes; i++) {
                // Middle Children
                if (pipe(pipefds[i]) < 0) {
                    perror("\nPipe could not be initialized");
                    return;
                }

                // Fork Child 3
                fflush(stdout);
                fflush(stderr);
                p_child = fork();
                if (p_child < 0) {
                    printf("\nCould not fork");
                    return;
                }

                // Loop will continue for forked child, which will increment i and continue on
                if (p_child != 0) {
                    // Child 2 Executable Code

#ifdef DEBUG
                    printf("Parent Pipe %d: %d\n", pipefds[i - 1][0], pipefds[i - 1][1]);
                    printf("Pipe %d: %d\n", pipefds[i][0], pipefds[i][1]);
                    fflush(stdout);
#endif

                    dup2(pipefds[i][0], STDIN_FILENO);  // From i+1 Child
                    dup2(pipefds[i - 1][1], STDOUT_FILENO);  // To i - 1 Child
                    if (pipe_types[i] == 1) dup2(pipefds[i - 1][1], STDERR_FILENO);

                    close(pipefds[i - 1][0]);
                    close(pipefds[i][0]);
                    close(pipefds[i - 1][1]);
                    close(pipefds[i][1]);

                    if (execvp(arg_sets[num_pipes - i][0], arg_sets[num_pipes - i]) < 0) {
                        printf("\nCould not execute command %d - %s", i, arg_sets[num_pipes - i][0]);
                        exit(0);
                    }
                }
            }

            // Last Child Executable Code

            // It only needs to write at the write end
            dup2(pipefds[num_pipes - 1][1], STDOUT_FILENO);
            if (pipe_types[0] == 1) dup2(pipefds[num_pipes - 1][1], STDERR_FILENO);
            close(pipefds[num_pipes - 1][0]);
            close(pipefds[num_pipes - 1][1]);

            // File Input Redirection
            if (FLAG_DETACH && !FLAG_IN_REDIR) {
                // Redirect Input to /dev/null for Detached Child
                FLAG_IN_REDIR = TRUE;
                input_file = "/dev/null";
            }

            if (FLAG_IN_REDIR) {
                if ((input_fd = open(input_file, FIN_FLAGS, S_IRUSR | S_IWUSR)) < 0) {
                    perror("Could not set input redirection file");
                    return;
                }
                dup2(input_fd, STDIN_FILENO);
                if (close(input_fd) != 0) perror("Could not close Input File Descriptor");
            }

            // Execute Last Child Command
            if (execvp(arg_sets[0][0], arg_sets[0]) < 0) {
                printf("\nCould not execute command 1 - %s", arg_sets[0][0]);
                exit(0);
            }
        } else {
            // Child 1 Executable Code
            // It only needs to read at the read end

            dup2(pipefds[0][0], STDIN_FILENO);

            close(pipefds[0][1]);
            close(pipefds[0][0]);

            // File Output Redirection
            if (FLAG_OUT_REDIR) {
                if ((output_fd = open(output_file, FOUT_FLAGS, S_IRUSR | S_IWUSR)) < 0) {
                    perror("Could not set output redirection file");
                    return;
                }

                dup2(output_fd, STDOUT_FILENO);
                if (close(output_fd) != 0) perror("Could not close Output File Descriptor");
            }

            if (execvp(arg_sets[num_pipes][0], arg_sets[num_pipes]) < 0) {
                printf("\nCould not execute command %d - %s", num_pipes + 1, arg_sets[num_pipes][0]);
                exit(0);
            }
        }
    } else if (!FLAG_DETACH) {
        // Wait for forked child process to complete if process is attached to parent
        pid_t completed_child = 0;
        while (completed_child != p_child) {
            completed_child = wait(NULL);
#ifdef DEBUG
            printf("PID %d just completed\n", completed_child);
#endif
        }
    } else {
        // Print PID for child task
        printf("%s [%d]\n", arg_sets[0][0], p_child);
    }
}

/**
 * Handle Built-in commands, like MV, cd, etc.
 */
int builtin_handler(char **args, int arg_len) {
    int NoOfBuiltinCmds = 4, i, switchBuiltinArg = 0;
    char *ListOfBuiltinCmds[NoOfBuiltinCmds];
    int num_effective_args = 0;
    char *mv_args[2];
    int j = 0;
    int FLAG = FALSE;

    ListOfBuiltinCmds[0] = "exit";
    ListOfBuiltinCmds[1] = "cd";
    ListOfBuiltinCmds[2] = "MV";
    ListOfBuiltinCmds[3] = "help";

#ifdef DEBUG
    int k;
    printf("%d arguments - ", arg_len);
    for (k = 0; k < arg_len; ++k) {
        printf("%s, ", args[k]);
    }
    printf("\n");
#endif

    for (i = 0; i < NoOfBuiltinCmds; i++) {
        if (strcmp(args[0], ListOfBuiltinCmds[i]) == 0) {
            switchBuiltinArg = i + 1;
            break;
        }
    }

    switch (switchBuiltinArg) {
        case 1:
            printf("\nGoodbye!\n");
            _exit(0);
        case 2:
            if (arg_len < 2) chdir(getenv("HOME"));
            else if (arg_len > 2) fprintf(stderr, "Too Many Args for cd\n");
            else {
                if (chdir(args[1]) != 0)
                    perror("Invalid Directory");
            }
            return 1;
        case 3:
            // Skip first, which is MV
            for (i = 1; i < arg_len; i++) {
                if (strcmp(args[i], "-n") == 0)
                    FLAG = FALSE;
                else if (strcmp(args[i], "-f") == 0)
                    FLAG = TRUE;
                else {
                    num_effective_args++;
                    if (j < 2) {
                        mv_args[j++] = args[i];
                    }
                }
            }

            if (num_effective_args > 2) {
                fprintf(stderr, "Too Many Args for MV\n");
                return 1;
            } else if (num_effective_args < 2) {
                fprintf(stderr, "Too Few Args for MV\n");
                return 1;
            }

            mv(mv_args[0], mv_args[1], FLAG);
            return 1;
        case 4:
            openHelp();
            return 1;
        default:
            break;
    }

    return 0;
}

/**
 * Show Shell Help Text
 */
void openHelp() {
    printf("\n***WELCOME TO P2 SHELL HELP***"
           "\n-Use the shell at your own risk..."
           "\nList of Builtin Commands supported:"
           "\n>cd"
           "\n>MV"
           "\n>exit"
           "\n>all other general commands available in UNIX shell"
           "\n>pipe handling"
           "\n>improper space handling\n");
}

/**
 * File Move Built-In Command
 */
void mv(char *source, char *destination, int FLAG_FORCE) {
#ifdef DEBUG
    printf("Source - %s\n", source);
    printf("Destination - %s\n", destination);
#endif

    if (link(source, destination) == 0) {
        if (unlink(source) != 0) {
            perror("Cannot unlink source file\n");
        }
    } else if (is_directory(destination)) {
        // Moving file to a directory
        if (link(source, strcat(destination, file_name_from_path(source))) == 0) {
            if (unlink(source) != 0) {
                perror("Cannot unlink source file\n");
            }
        } else if (FLAG_FORCE) {
            if (unlink(strcat(destination, file_name_from_path(source))) != 0)
                perror("Cannot unlink destination file for overwriting\n");
            else
                mv(source, strcat(destination, file_name_from_path(source)), FALSE);
        } else {
            perror("Cannot move file - cannot establish link\n");
        }
    } else if (FLAG_FORCE) {
        // Current Destination File Exists
        if (unlink(destination) != 0)
            perror("Cannot unlink destination file for overwriting\n");
        else
            mv(source, destination, FALSE);
    } else {
        perror("Cannot move file - cannot establish link\n");
    }
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/**
 * Signal Handler - Catch Signals sent to the shell and ignore them, since we don't want to accidentally kill our shell.
 */
void signal_handler(int signum) {
#ifdef DEBUG
    printf("Received SIGTERM (%d), and the special handler is running...\n", signum);
#endif
}

#pragma clang diagnostic pop

int is_directory(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0)
        return 0;
    return S_ISDIR(statbuf.st_mode);
}

char *file_name_from_path(char *path) {
    char *fn;
    int offset = 0;

    if (path[(strlen(path) - 1)] == '/')
        offset = 1;

    (fn = strrchr(path + offset, '/')) ? ++fn : (fn = path);

    return fn;
}