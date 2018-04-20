//
// Created by Tom Paulus on 1/26/18.
// CS 570 -- Carroll
// Due: 2/9/2018 11 PM
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
#include <string.h>
#include "getword.h"

int is_meta_char(int c);

int is_meta_char_str(char c[]);

int getword(char *w) {
    int word_length = 0;
    int return_value = 0;
    int iochar;
    int meta_char;
    int prev_iochar = EOF;

    short escape_mode = FALSE;

    while ((iochar = getchar()) != EOF) {
        if (escape_mode && (iochar == SPACE || iochar == NEW_LINE)) {
            // Escape Character wasn't actually meant to escape a character
            *w = ((char) ESCAPE);
            w++;
            word_length++;
            escape_mode = FALSE;
        } else if (word_length == 0 && iochar == SPACE) continue; // Remove Initial Whitespace

// ==== Process Meta Characters ====
        meta_char = is_meta_char(iochar);
        if (!escape_mode && ((word_length > 0 && meta_char) || (word_length < 0 && !meta_char))) {
            // Switch Between Meta and Regular Chanters - Stopping
            ungetc(iochar, stdin);
            return_value = word_length;

            break;
        } else if (!escape_mode && meta_char) {
            char last_two[3] = {(char) prev_iochar, (char) iochar, ((char) 0)};
            if (word_length == -1 && !is_meta_char_str(last_two)) {
                // Not a valid meta character string - stopping
                ungetc(iochar, stdin);
                return_value = word_length;

                break;
            }

            word_length--;
            *w = ((char) iochar);
            w++;
            escape_mode = FALSE;

//  ==== Process Regular Characters ====
        } else if (!escape_mode && iochar == ESCAPE) {
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
        } else if (word_length == 0 && iochar == NEW_LINE) {
            //Single New Line
            return_value = -10;

            break;
        } else if ((!escape_mode && iochar == SPACE) || iochar == NEW_LINE) {
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

            if (abs(word_length) > STORAGE - 2) {
                // Maximum length of storage array has been reached. Stopping Input Loop
                return_value = word_length;
                break;
            }
        }

        prev_iochar = iochar;
    }


    if (word_length != 0 && iochar == EOF) {
        ungetc(iochar, stdin);
        return_value = word_length;
    }

    *w = ((size_t) NULL); // Don't forget to Null Terminate!

    return return_value;
}

int is_meta_char(int c) {
    const int num_meta_chars = 5;
    const char meta_chars[] = {'<', '>', '|', '&'};

    int i;
    for (i = 0; i < num_meta_chars; ++i) {
        if (meta_chars[i] == c) return TRUE;
    }
    return FALSE;
}

int is_meta_char_str(char c[]) {
    const int num_meta_char_strs = 1;
    const char *meta_char_strs[] = {"|&"};

    int i;
    for (i = 0; i < num_meta_char_strs; ++i) {
        if (strcmp(c, meta_char_strs[i]) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}