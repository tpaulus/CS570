//
// Created by Tom Paulus on 1/18/18.
//

#define COMMENT_MARKER '#'
#define SPACE ' '
#define NEW_LINE '\n'
#define NULL '\0'

#include <stdio.h>
#include "getword.h"

/**
 * Get a Word from the Standard In
 *
 * @param w Result Array
 * @return -10 for New Line; -1 for Comment (#); 0 for EOF; else Word Length
 */
int getword(char *w) {
    int word_length = 0;
    int return_value = 0;
    int iochar;

    while ((iochar = getchar()) != EOF) {
        if (word_length == 0 && iochar == SPACE) continue; // Remove Initial Whitespace
        else if (word_length == 0 && iochar == COMMENT_MARKER) {
            // Comment marker at start of line
            return_value = -1;

            // Save character in result array
            *w = ((char) iochar);
            w++;

            break;
        } else if (word_length == 0 && iochar == NEW_LINE) {
            //Single New Line
            return_value = -10;

            break;
        } else if (iochar == SPACE || iochar == NEW_LINE) {
            // End of Word, Return Length
            return_value = word_length;

            // Put Back the newline for the Next Word
            if (iochar == NEW_LINE) ungetc(iochar, stdin);
            break;
        } else {
            // Save character in result array
            *w = ((char) iochar);
            w++;
            word_length++;
        }
    }

    if (word_length != 0 && iochar == EOF) {
        ungetc(iochar, stdin);
        return_value = word_length;
    }

    *w = ((char) NULL); // Don't forget to Null Terminate!
    // fixme getword.c:60:11: warning: cast from pointer to integer of different size [-Wpointer-to-int-cast]

    return return_value;
}
