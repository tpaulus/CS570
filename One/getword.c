//
// Created by Tom Paulus on 1/26/18.
// CS 570 -- Carroll
// Due: 1/25/2018 11 PM
// TODO Fix Due Date when posted
//

#define COMMENT_MARKER '#'
#define ESCAPE '\\'
#define SPACE ' '
#define NEW_LINE '\n'
#define NULL '\0'

#define FALSE 0
#define TRUE 1

#include <stdio.h>
#include <stdlib.h>
#include "getword.h"

/**
 * Check if a given character is a Meta Character
 *
 * @param c Character
 * @return 1 if a Meta Character; 0 otherwise
 */
short is_meta_char(int c) {
    const int num_meta_chars = 5;
    const char meta_chars[] = {'<', '>', '|', '&', '#'};

    int i = 0;
    for (i; i < num_meta_chars; ++i) {
        if (meta_chars[i] == c) return TRUE;
    }
    return FALSE;
}

/**
 * Get a Word from the Standard In
 *
 * @param w Result Array
 * @return -10 for New Line; 0 for EOF; else Word Length, or negative length for Meta Characters
 */
int getword(char *w) {
    int word_length = 0;
    int return_value = 0;
    int iochar;

    short escape_mode = FALSE;

    while ((iochar = getchar()) != EOF) {
        if (word_length == 0 && iochar == SPACE) continue; // Remove Initial Whitespace

        // Process Meta Characters
        short isMetaChar = is_meta_char(iochar);
        if (!escape_mode && ((word_length > 0 && isMetaChar) || (word_length < 0 && !isMetaChar))) {
            // Switch Between Meta and Regular Chanters - Stopping
            ungetc(iochar, stdin);
            return_value = word_length;

            break;
        } else if (!escape_mode && isMetaChar) {
            word_length--;
            *w = ((char) iochar);
            w++;
            escape_mode = FALSE;

            continue;
        }

        // Process Regular Characters
        if (!escape_mode && iochar == ESCAPE) {
            // Process / Markers
            escape_mode = TRUE;
            continue;
        } else if (!escape_mode && word_length == 0 && iochar == COMMENT_MARKER) {
            // Comment marker at start of line
            return_value = -1;

            // Save character in result array
            *w = ((char) iochar);
            w++;

            break;
        } else if (!escape_mode && word_length == 0 && iochar == NEW_LINE) {
            //Single New Line
            return_value = -10;

            break;
        } else if (!escape_mode && (iochar == SPACE || iochar == NEW_LINE)) {
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
            escape_mode = FALSE;

            if (abs(word_length) > STORAGE - 1) {
                // Maximum length of storage array has been reached. Stopping Input Loop
                break;
            }
        }
    }

    if (word_length != 0 && iochar == EOF) {
        ungetc(iochar, stdin);
        return_value = word_length;
    }

    *w = ((size_t) NULL); // Don't forget to Null Terminate!

    return return_value;
}
