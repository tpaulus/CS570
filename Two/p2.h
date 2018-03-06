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
 */
void mv(char *source, char *destination);