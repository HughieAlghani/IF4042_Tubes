#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/utility/make_trie.c,v 11.0 1992/07/21 18:19:32 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include <ctype.h>
#include "common.h"
#include "param.h"
#include "functions.h"
#include "trie.h"

/* Read an input file in the format given below, constructing a C source
   file that will compile to a trie representing the values given in the
   file.

   Input file (read from standard input) format:
   First line gives the trie array name. Must be legal C characters, with
   no blanks.
   Rest of lines are of form:
         string  unsigned_long_integer
   where string may be composed of any ascii character except NUL or newline,
   Whitespace between values may be either blanks or tabs.  Input file
   (except first line) MUST be sorted in ascii increasing order.

   Output file (written on standard output) format:
   Explanatory comment
   unsigned long trie_name[] = {
       (longs representing trie)
   };
   where trie_name was given by first line of input.
*/

/*
   Trie is represented by an array of longs which contain a compact
   hierarchy interspersed with the values for string. In a hierarchy node,
   the right 7 bits represent the trie component, the next rightmost bit tells
   whether this node has a value.  The leftmost 24 bits give an integer
   offset into the trie structure, giving the starting location for the
   children of this node.  If these 24 bits are 0, then there are no
   children.  The end location for the a node's children is indicated by
   a node with ascii code 0 (ie, the rightmost 7 bits are 0).  NULL (0) is
   the only 7 bit ascii character not allowed in the strings.
*/

char * explanation = 
"/* This file gives C source code for the values of a trie data structure.\n\
   It was constructed automatically by make_trie */\n\n";


typedef struct {
    char *string;           
    unsigned long value;
} TOKEN;

TOKEN *tokens;

TRIE *root_trie;                /* base of trie being established */
TRIE *last_node;                /* Last trie node written */

static void read_tokens_and_init(), enter_tokens(), write_trie();

void
main ()
{
    char trie_name[80];

    (void) gets (trie_name);

    read_tokens_and_init();

    enter_tokens (tokens, 0, root_trie);

    write_trie (trie_name);

    exit (0);
}

#define SIZE_STR_MALLOC 8000
#define SIZE_TOKEN_MALLOC 250

static void
read_tokens_and_init()
{
    register long num_tokens;
    register char *str_ptr;

    long end_token;
    char *end_str_ptr;

    int num_mallocs = 0;         /* num_mallocs * SIZE_STR_MALLOC gives
                                    bound on number characters read; used
                                    to bound size of trie needed */

    num_tokens = 0;
    tokens = (TOKEN *) malloc (SIZE_TOKEN_MALLOC * sizeof (TOKEN));
    end_token = SIZE_TOKEN_MALLOC;

    str_ptr = malloc (SIZE_STR_MALLOC);
    end_str_ptr = &str_ptr [SIZE_STR_MALLOC];
    num_mallocs++;

    while (2 == scanf ("%s %lu\n", str_ptr, &tokens[num_tokens].value)) {
        tokens[num_tokens].string = str_ptr;
#ifdef DEBUG
        (void) fprintf (stderr, "Read token '%s' '%lu'\n",
                 tokens[num_tokens].string, tokens[num_tokens].value);
#endif DEBUG
        num_tokens++;
        if (num_tokens >= end_token) {
            tokens = (TOKEN *) realloc ((char *) tokens,
                                        (unsigned) (num_tokens + SIZE_TOKEN_MALLOC)
                                          * sizeof (TOKEN));
            end_token = num_tokens + SIZE_TOKEN_MALLOC;
        }

        while (*str_ptr++)
            ;
        if (end_str_ptr - str_ptr < MAX_SIZE_STRING) {
            str_ptr = malloc (SIZE_STR_MALLOC);
            end_str_ptr = &str_ptr[SIZE_STR_MALLOC];
            num_mallocs++;
        }
    }
    tokens[num_tokens].string = NULL;

    /* Reserve space for the complete trie */
    if (NULL == (root_trie = (TRIE *)
                 calloc ((unsigned)(SIZE_STR_MALLOC * num_mallocs+ num_tokens),
                         sizeof (TRIE)))) {
        (void) fprintf (stderr,
                    "make_trie: Can't reserve enough space for trie - Quit\n");
        exit (2);
    }
    last_node = root_trie;
}        


/* enter all the tokens that agree with start_string up through the
   first prefix_length characters.  Trie gives the trie node for
   string start_string[0..prefix_length-1]. 

   First construct (and reserve space for ) the children level of trie
   by going through strings, looking for possible next character.  Then
   go through strings second time, constructing the sub-hierarchy
   for each child.
*/
static void
enter_tokens (start_token, prefix_length, trie)
register TOKEN *start_token;
int prefix_length;
register TRIE *trie;
{
    register TOKEN *ptr;
    char char_ptr;                    /* next character to be inserted as
                                         child of trie */
    ptr = start_token;
#ifdef DEBUG
        (void) fprintf (stderr, "enter_tokens: '%s', %ld, %d\n",
                        ptr->string, prefix_length, trie - root_trie);
#endif DEBUG
    while (ptr->string &&
           ! strncmp (start_token->string, ptr->string, prefix_length)) {
        char_ptr = ptr->string[prefix_length];
        last_node++;
        if (! has_children (trie) && ! has_value (trie)) {
            set_children_ptr (trie, last_node - trie);
        }
        if (char_ptr) {
            set_has_children (trie, 1);
            set_ascii_char (last_node, char_ptr);
        }
        else {
            /* relies on fact that char_ptr == '\0' occurs before children */
            set_has_value (trie, 1);
            set_value (last_node, ptr->value);
        }
        /* Skip over all strings which match through char_ptr */
        while (ptr->string &&
               ! strncmp (start_token->string, ptr->string, prefix_length) &&
               ptr->string[prefix_length] == char_ptr) {
            ptr++;
        }
    }
    if (has_children (trie)) {
        set_last_child (last_node, 1);
    }

#ifdef DEBUG
        (void) fprintf (stderr, "  Trie = %lx\n", *trie);
#endif DEBUG
    /* Pass 2 over tokens.  Enter the sub-hierarchy starting at each child
       of trie
    */
    ptr = start_token;
    if (has_children (trie)) {
        /* Skip over any entries ending word */
        while (ptr->string &&
               ! strncmp (start_token->string, ptr->string, prefix_length) &&
               ptr->string[prefix_length] == '\0') {
            ptr++;
        }

        trie = trie + children_ptr (trie) + (has_value(trie) ? 1 : 0);
        do {
            enter_tokens (ptr, prefix_length+1, trie);
            while (ptr->string &&
                   !strncmp(start_token->string, ptr->string, prefix_length) &&
                   ptr->string[prefix_length] == ascii_char (trie)) {
                ptr++;
            }
        } while (! last_child (trie++));
    }
}


static void
write_trie(trie_name)
char *trie_name;
{
    TRIE *ptr;
    
    puts ("#include \"trie.h\"\n");
    puts (explanation);

    (void) printf ("TRIE %s[] = {", trie_name);

    put_ascii (root_trie);
    for (ptr = root_trie + 1; ptr <= last_node; ptr++) {
        puts (",");
        put_ascii (ptr);
    }

    puts ("\n};");
}
