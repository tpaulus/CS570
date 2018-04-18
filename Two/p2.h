#include <stdio.h>
#include "getword.h"

#define MAXITEM 100 /* max number of words per line */

/**
 * Get input line from STDIN via getword()
 *
 * @param args Restulting Args Array
 * @return Number of words (args) supplied by the user
 */
int parse(char **args);

/**
 * Execute Built-in commands, like CD, MV, etc.
 *
 * @param args  Input Args Array
 * @param arg_len Number of Args
 * @return 0 if builtin-command executed, 1 if no command was found
 */
int builtin_handler(char **args, int arg_len);

/**
 * Execute a Simple Command (No Pipe)
 * @param args Command and Arguments
 * @param arg_len Number of Args
 */
void exec_simple(char **args, int arg_len);

/**
 * Execute a Piped Set of Commands
 *
 * @param args1 First Command - Writes to the Pipe
 * @param args2 Second Command - Reads from the Pipe
 */
void exec_piped(char **args1, char **args2);

/**
 * Prepare the shell for the next line to be supplied by the user.
 */
void init();

/**
 * Catch Signals sent to the Shell
 * @param signum Signal Number
 */
void signal_handler(int signum);

//  ==== Built-in Commands ====

/**
 * Show Shell Help Text
 */
void openHelp();

/**
 * File Move Built-In Command
 *
 * @param source   Source File Path
 * @param destination Destination File Path
 * @param FLAG_FORCE 1 if the move should be forced, else 0
 */
void mv(char *source, char *destination, int FLAG_FORCE);

//  ==== Helper Functions ====

/**
 * Check whether or not a given path is a directory.
 *
 * @param path Path Char Array
 * @return 1 if directory; else 0
 */
int is_directory(const char *path) ;

/**
 * Get the file name from a given path. Useful for determining the file name in a move command.
 *
 * @param path Path Char Array
 * @return Char Array of the File Name (what follows the last '/')
 */
char* file_name_from_path(char *path);