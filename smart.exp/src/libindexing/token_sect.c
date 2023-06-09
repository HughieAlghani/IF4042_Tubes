#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libindexing/token_sect.c,v 11.0 1992/07/21 18:21:39 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/
/********************   PROCEDURE DESCRIPTION   ************************
 *0 Construct tokens from a single preparsed document section
 *1 index.token_sect.token_sect
 *2 token_sect (pp_buf, t_sect, inst)
 *3   SM_BUF *pp_buf;
 *3   SM_TOKENSECT *t_sect;
 *3   int inst;
 *4 init_token_sect (spec, unused)
 *5   "*.interior_char"
 *5   "*.interior_hyphenation"
 *5   "*.endline_hyphenation"
 *5   "index.token_sect.trace"
 *4 close_token_sect (inst)

 *7 Take the preparsed section pp_buf, and construct a list of tokens 
 *7 and their types in it, returning the list in t_sect.  Interior_char
 *7 gives a list of characters (eg "'") that should be considered as
 *7 part of a text word (beginning with a letter), assuming that they
 *7 are followed by other letters or digits.  
 *7 Hyphenation indicates what should be
 *7 done with hyphens interior to normal words (directly surrounded
 *7 by letters) or at the end of a line.  If TRUE, then hyphens (and possibly
 *7 the new line) are removed, otherwise hyphens are treated as ordinary
 *7 characters.  If hyphen is an interior_char, then that treatment takes
 *7 precedent.
 *7 The list of tokens obtained from pp_buf is guaranteed 
 *7 sufficient to reconstruct pp_buf (eg, could be used for compression) 
 *7 except if either hyphenation is turned on.
 *7 1 is returned on normal completion, UNDEF if error.

 *8 From docindex.h:
 *8 Output of tokenizer, giving the tokens found in the text of a doc, along 
 *8    with the tokentype *
 *8 typedef struct {
 *8     char *token;
 *8     long tokentype;
 *8 } SM_TOKENTYPE;                     * Individual token *
 *8 
 *8 typedef struct {
 *8     long section_num;
 *8     long num_tokens;
 *8     SM_TOKENTYPE *tokens;
 *8 } SM_TOKENSECT;                     * Tokens for a section *
 *8 * Tokentype can take on one of the following values in the standard parser*
 *8 * Other parsers may use other values in addition.  By convention, positive
 *8    values indicate that particular tokentype should be used as ctype *
 *8 * Token is all lower case letters *
 *8 #define SM_INDEX_LOWER -1
 *8 * Token is mixed case (Combination of upper, lower, digits)  (possibly
 *8    including any characters from token_sect.interior_char as long as a
 *8    another letter immediately follows).
 *8 #define SM_INDEX_MIXED -2
 *8 * Token is all digits (possibly including '.') *
 *8 #define SM_INDEX_DIGIT -3
 *8 * Punctuation *
 *8 #define SM_INDEX_PUNC -4
 *8 * Whitespace *
 *8 #define SM_INDEX_SPACE -5
 *8 * End of Sentence (Token = NULL) *
 *8 #define SM_INDEX_SENT -6
 *8 * End of Paragraph (Token = NULL) *
 *8 #define SM_INDEX_PARA -7
 *8 * Start of new section (Token[0] = section_id) *
 *8 #define SM_INDEX_SECT_START -8
 *8 * End of section (Token[0] = section_id) *
 *8 #define SM_INDEX_SECT_END -9
 *8 * Ignore this token completely *
 *8 #define SM_INDEX_IGNORE -10

 *9 Hyphens not handled correctly anymore?  Should they be handled here?
 *9 single case?
 *9 code for T_I
***********************************************************************/


#include <ctype.h>
#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "proc.h"
#include "spec.h"
#include "docindex.h"
#include "trace.h"
#include "inst.h"
#include "buf.h"

/* Lower case */
#define T_L  0
/* Upper case */
#define T_U  1
/* Space SP, TAB, CR, NL, FF */
#define T_SP 2
/* Digit */
#define T_D  3
/* Punctuation */
#define T_P 4
/* Plus and Minus (may begin number) */
#define T_DP 5
/* Control */
#define T_CN 6
/* Punctuation that can be interior to a word (eg "'") */
#define T_I 7

/*     Hexadecimal - Character

     | 00 NUL| 01 SOH| 02 STX| 03 ETX| 04 EOT| 05 ENQ| 06 ACK| 07 BEL|
     | 08 BS | 09 HT | 0A NL | 0B VT | 0C NP | 0D CR | 0E SO | 0F SI |
     | 10 DLE| 11 DC1| 12 DC2| 13 DC3| 14 DC4| 15 NAK| 16 SYN| 17 ETB|
     | 18 CAN| 19 EM | 1A SUB| 1B ESC| 1C FS | 1D GS | 1E RS | 1F US |
     | 20 SP | 21  ! | 22  " | 23  # | 24  $ | 25  % | 26  & | 27  ' |
     | 28  ( | 29  ) | 2A  * | 2B  + | 2C  , | 2D  - | 2E  . | 2F  / |
     | 30  0 | 31  1 | 32  2 | 33  3 | 34  4 | 35  5 | 36  6 | 37  7 |
     | 38  8 | 39  9 | 3A  : | 3B  ; | 3C  < | 3D  = | 3E  > | 3F  ? |
     | 40  @ | 41  A | 42  B | 43  C | 44  D | 45  E | 46  F | 47  G |
     | 48  H | 49  I | 4A  J | 4B  K | 4C  L | 4D  M | 4E  N | 4F  O |
     | 50  P | 51  Q | 52  R | 53  S | 54  T | 55  U | 56  V | 57  W |
     | 58  X | 59  Y | 5A  Z | 5B  [ | 5C  \ | 5D  ] | 5E  ^ | 5F  _ |
     | 60  ` | 61  a | 62  b | 63  c | 64  d | 65  e | 66  f | 67  g |
     | 68  h | 69  i | 6A  j | 6B  k | 6C  l | 6D  m | 6E  n | 6F  o |
     | 70  p | 71  q | 72  r | 73  s | 74  t | 75  u | 76  v | 77  w |
     | 78  x | 79  y | 7A  z | 7B  { | 7C  | | 7D  } | 7E  ~ | 7F DEL|
*/

static char sm_ctype[] = {
      T_CN,    T_CN,    T_CN,    T_CN,    T_CN,    T_CN,    T_CN,    T_CN,
      T_CN,    T_SP,    T_SP,    T_CN,    T_SP,    T_SP,    T_CN,    T_CN,
      T_CN,    T_CN,    T_CN,    T_CN,    T_CN,    T_CN,    T_CN,    T_CN,
      T_CN,    T_CN,    T_CN,    T_CN,    T_CN,    T_CN,    T_CN,    T_CN,
      T_SP,    T_P,     T_P,     T_P,     T_P,     T_P,     T_P,     T_P,
      T_P,     T_P,     T_P,     T_DP,    T_P,     T_DP,    T_P,     T_P,
      T_D,     T_D,     T_D,     T_D,     T_D,     T_D,     T_D,     T_D,
      T_D,     T_D,     T_P,     T_P,     T_P,     T_P,     T_P,     T_P,
      T_P,     T_U,     T_U,     T_U,     T_U,     T_U,     T_U,     T_U,
      T_U,     T_U,     T_U,     T_U,     T_U,     T_U,     T_U,     T_U,
      T_U,     T_U,     T_U,     T_U,     T_U,     T_U,     T_U,     T_U,
      T_U,     T_U,     T_U,     T_P,     T_P,     T_P,     T_P,     T_P,
      T_P,     T_L,     T_L,     T_L,     T_L,     T_L,     T_L,     T_L,
      T_L,     T_L,     T_L,     T_L,     T_L,     T_L,     T_L,     T_L,
      T_L,     T_L,     T_L,     T_L,     T_L,     T_L,     T_L,     T_L,
      T_L,     T_L,     T_L,     T_P,     T_P,     T_P,     T_P,     T_CN
      };


/* check for single case ?? */
/* check for interior punctuation */

static SPEC_PARAM spec_args[] = {
    TRACE_PARAM ("index.token_sect.trace")
    };
static int num_spec_args = sizeof (spec_args) / sizeof (spec_args[0]);

static char *prefix;
static char *interior;              /* Characters that are considered to
                                       be part of a word IF they occur in
                                       the interior of a word.  Eg "'". */
static long interior_hyphen_flag;   /* What should be done with hyphens 
                                       interior to words */
static long endline_hyphen_flag;    /* What should be done with hyphens at
                                       end of lines. */
                                    /* If either flag is TRUE, then hyphens 
                                       (and possibly newline) are removed */
static SPEC_PARAM_PREFIX prefix_spec_args[] = {
    &prefix,  "interior_char",   getspec_string, (char *) &interior,
    &prefix,  "interior_hyphenation", getspec_bool,
                                         (char *) &interior_hyphen_flag,
    &prefix,  "endline_hyphenation", getspec_bool,
                                         (char *) &endline_hyphen_flag,
    };
static int num_prefix_spec_args = sizeof (prefix_spec_args) / sizeof (prefix_spec_args[0]);

/* Static info to be kept for each instantiation of this proc */
typedef struct {
    /* bookkeeping */
    int valid_info;

    SM_TOKENTYPE *tokens;
    unsigned max_num_tokens;

    char *token_buf;
    unsigned size_token_buf;

    char *char_type;
    long interior_hyphen_flag;
    long endline_hyphen_flag;
} STATIC_INFO;

static STATIC_INFO *info;
static int max_inst = 0;


int
init_token_sect (spec, param_prefix)
SPEC *spec;
char *param_prefix;
{
    STATIC_INFO *ip;
    int new_inst;
    char *ptr;

    /* Lookup the values of the relevant parameters */
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args)) {
        return (UNDEF);
    }
    prefix = param_prefix;
    if (UNDEF == lookup_spec_prefix (spec,
                                     &prefix_spec_args[0],
                                     num_prefix_spec_args))
        return (UNDEF);
    PRINT_TRACE (2, print_string, "Trace: entering init_token_sect");

    NEW_INST (new_inst);
    if (UNDEF == new_inst)
        return (UNDEF);

    ip = &info[new_inst];

    ip->size_token_buf = 0;
    ip->max_num_tokens = 0;
    ip->endline_hyphen_flag = endline_hyphen_flag;
    ip->interior_hyphen_flag = interior_hyphen_flag;

    if (NULL == (ip->char_type = malloc (128)))
        return (UNDEF);
    bcopy (sm_ctype, ip->char_type, 128);

    for (ptr = interior; *ptr; ptr++) {
        if (isascii (*ptr)) 
            ip->char_type[*ptr] = T_I;
    }

    ip->valid_info = 1;

    PRINT_TRACE (2, print_string, "Trace: leaving init_token_sect");

    return (new_inst);
}


int
token_sect (pp_buf, t_sect, inst)
SM_BUF *pp_buf;                    /* Input document section*/
SM_TOKENSECT *t_sect;              /* Output tokens */
int inst;
{
    STATIC_INFO *ip;
    unsigned size_needed;
    char *ptr, *end_ptr;
    char *buf_ptr;
    int mixed_token;
    int mixed_newline = 0;

    PRINT_TRACE (2, print_string, "Trace: entering token_sect");
    PRINT_TRACE (6, print_buf, pp_buf);

     if (! VALID_INST (inst)) {
        set_error (SM_ILLPA_ERR, "Instantiation", "skel");
        return (UNDEF);
    }
    ip  = &info[inst];

   /* Determine maximum size of buffer needed to hold all the document's
       tokens. */
    size_needed =  5 + 2 * pp_buf->end;
    if (size_needed > ip->size_token_buf) {
        if (0 != ip->size_token_buf)
            (void) free (ip->token_buf);
        ip->size_token_buf += size_needed;
        if (NULL == (ip->token_buf = malloc ((unsigned) ip->size_token_buf))) {
            set_error (errno, "Out of space", "token_sect");
            return (UNDEF);
        }
    }
    if (size_needed > ip->max_num_tokens) {
        if (0 != ip->max_num_tokens)
            (void) free ((char *) ip->tokens);
        ip->max_num_tokens += size_needed;
        if (NULL == (ip->tokens = (SM_TOKENTYPE *)
                     malloc (ip->max_num_tokens * sizeof (SM_TOKENTYPE)))) {
            set_error (errno, "Out of space", "token_sect");
            return (UNDEF);
        }
    }

    t_sect->num_tokens = 0;
    t_sect->tokens = ip->tokens;
    buf_ptr = ip->token_buf;

    ptr = pp_buf->buf;
    end_ptr = &pp_buf->buf[pp_buf->end];

    while (ptr < end_ptr) {
        t_sect->tokens[t_sect->num_tokens].token = buf_ptr;
        if (! isascii (*ptr)) {
            /* ignore */
            *buf_ptr++ = *ptr++;
            *buf_ptr++ = '\0';
            t_sect->tokens[t_sect->num_tokens].tokentype = SM_INDEX_IGNORE;
            continue;
        }
        switch (ip->char_type[*ptr]) {
          case T_U:
          case T_L:
            if (ip->char_type[*ptr] == T_U)
                mixed_token = 1;
            else
                mixed_token = 0;
            *buf_ptr++ = *ptr++;
            while (ptr < end_ptr) {
                if (ip->char_type[*ptr] == T_L) {
                    *buf_ptr++ = *ptr++;
                }
                else if (ip->char_type[*ptr] == T_U ||
                         ip->char_type[*ptr] == T_D) {
                    mixed_token = 1;
                    *buf_ptr++ = *ptr++;
                }
                else if (ip->char_type[*ptr] == T_I &&
                         ptr+1 < end_ptr &&
                         (ip->char_type[*(ptr+1)] == T_L ||
                          ip->char_type[*(ptr+1)] == T_U ||
                          ip->char_type[*(ptr+1)] == T_D)) {
                    mixed_token = 1;
                    *buf_ptr++ = *ptr++;
                }
                else if (*ptr == '-') {
                    if (ip->interior_hyphen_flag &&
                        ptr+1 < end_ptr &&
                        (ip->char_type[*(ptr+1)] == T_L ||
                         ip->char_type[*(ptr+1)] == T_U)) {
                        /* Simply skip over hyphen character in text */
                        ptr++;
                    }
                    else if (ip->endline_hyphen_flag &&
                             ptr+2 < end_ptr &&
                             *(ptr+1) == '\n' &&
                             (ip->char_type[*(ptr+2)] == T_L ||
                              ip->char_type[*(ptr+2)] == T_U)) {
                        /* Skip over hyphen character and newline in text */
                        ptr++; ptr++;
                    }
                    else
                        break;
                }
                else
                    break;
            }
            t_sect->tokens[t_sect->num_tokens].tokentype = (mixed_token ?
                                                            SM_INDEX_MIXED :
                                                            SM_INDEX_LOWER);
            break;
          case T_SP:
            do {
                if (*ptr == '\n')
                    mixed_newline = 1;
                *buf_ptr++ = *ptr++;
            } while (ptr < end_ptr && ip->char_type[*ptr] == T_SP);
            if (mixed_newline) {
                t_sect->tokens[t_sect->num_tokens].tokentype =
                    SM_INDEX_NLSPACE;
                mixed_newline = 0;
            }
            else {
                t_sect->tokens[t_sect->num_tokens].tokentype = SM_INDEX_SPACE;
            }
            break;
          case T_DP:
            if ((ptr+1) >= end_ptr || (ip->char_type[*(ptr+1)] != T_D)) {
                *buf_ptr++ = *ptr++;
                t_sect->tokens[t_sect->num_tokens].tokentype = SM_INDEX_PUNC;
                break;
            }
          case T_D:
            do {
                *buf_ptr++ = *ptr++;
            } while (ptr < end_ptr && (ip->char_type[*ptr] == T_D ||
                                       ((*ptr == '.' || *ptr == ',') &&
                                        (ptr+1) < end_ptr &&
                                        ip->char_type[*(ptr+1)] == T_D)));
            t_sect->tokens[t_sect->num_tokens].tokentype = SM_INDEX_DIGIT;
            break;
          case T_P:
          case T_I:
            *buf_ptr++ = *ptr++;
            t_sect->tokens[t_sect->num_tokens].tokentype = SM_INDEX_PUNC;
            break;
          case T_CN:
          default:
            *buf_ptr++ = *ptr++;
            t_sect->tokens[t_sect->num_tokens].tokentype = SM_INDEX_IGNORE;
            break;
        }

        /* Null terminate the current token */
        *buf_ptr++ = '\0';
        t_sect->num_tokens++;
    }

    PRINT_TRACE (4, print_tokensect, t_sect);
    PRINT_TRACE (2, print_string, "Trace: leaving token_sect");

    return (0);
}

int
close_token_sect (inst)
int inst;
{
    STATIC_INFO *ip;

    PRINT_TRACE (2, print_string, "Trace: entering close_token_sect");

    if (! VALID_INST (inst)) {
        set_error (SM_ILLPA_ERR, "Instantiation", "close_token_sect");
        return (UNDEF);
    }

    ip  = &info[inst];
    ip->valid_info--;

    if (ip->valid_info == 0) {
        if (0 != ip->size_token_buf)
            (void) free (ip->token_buf);
        if (0 != ip->max_num_tokens)
            (void) free ((char *) ip->tokens);
        (void) free (ip->char_type);
    }

    PRINT_TRACE (2, print_string, "Trace: leaving close_token_sect");

    return (0);
}

