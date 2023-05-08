#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libindexing/parse_fulla.c,v 11.0 1992/07/21 18:21:36 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Parse a standard text section, returning concepts and location info
 *1 index.parse_sect.full_all
 *2 full_all_parse (token, consect, inst)
 *3   SM_TOKENSECT *token;
 *3   SM_CONSECT *consect;
 *3   int inst;
 *4 init_full_all_parse(spec, unused)
 *5   "index.parse_sect.trace"
 *5   "index.parse.single_case"
 *5   "index.parse.section.<token->section_num>.{word,proper,number}.ctype"
 *5   "index.parse.section.<token->section_num>.{word,proper,number}.stemmer"
 *5   "index.parse.section.<token->section_num>.{word,proper,number}.stopword"
 *5   "index.parse.section.<token->section_num>.{word,proper,number}.token_to_con"
 *4 close_full_all_parse(inst)
 *7 Take the tokens given by token and produce a set of corresponding concepts
 *7 Normalize token to be all lower case, call appropriate stopword checker,
 *7 appropriate stemmer, convert to concept, and assign appropriate ctype.
 *7 Keep track of number of word, sentence, and paragraph seen.
 *8 * Input of section parser, giving the tokens found in the text of a 
 *8 doc, along with the tokentype *
 *8 typedef struct {
 *8     char *token;
 *8     long tokentype;
 *8 } SM_TOKENTYPE;                     * Individual token *
 *8 
 *8 typedef struct {
 *8     long section_num;               * Index of the section_id for section *
 *8     long num_tokens;
 *8     SM_TOKENTYPE *tokens;
 *8 } SM_TOKENSECT;                     * Tokens for a section *
 *8 
 *8 * Output of section parser *
 *8 typedef struct {
 *8     long con;                   * Concept number of input token *
 *8     long ctype;                 * Ctype of con *
 *8     long para_num;              * Paragraph within section *
 *8     long sent_num;              * Sentence number within section *
 *8     long token_num;             * Token number within section *
 *8 } SM_CONLOC;
 *8 
 *8 typedef struct {
 *8     long section_num;
 *8     long num_concepts;
 *8     SM_CONLOC *concepts;
 *8 } SM_CONSECT;

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

static int single_case_flag;     /* set if collection is all one case,
                                    so case cannot be used when checking
                                    for sentence boundaries */

static SPEC_PARAM spec_args[] = {
    TRACE_PARAM ("index.parse_sect.trace")
    "index.parse.single_case",    getspec_bool,  (char *) &single_case_flag,
    };
static int num_spec_args = sizeof (spec_args) / sizeof (spec_args[0]);

/* Each of word, proper, and number (the three parsetypes recognized by
   parse_full), will have a PARSETYPE_FUNC structure defined for it, which
   gives the operations to be done on it (gotten either directly from the
   spec file, or indirectly through the standard doc_desc structure).*/
typedef struct {
    PROC_INST stemmer;
    PROC_INST stopword;
    long ctype;
    PROC_INST token_to_con;
} PARSETYPE_FUNC;

#define SM_PT_F_WORD  0
#define SM_PT_F_PROPER  1
#define SM_PT_F_NUMBER  2

#define SM_PF_NUM_PF 3

/* Static info to be kept for each instantiation of this proc */
typedef struct {
    /* bookkeeping */
    int valid_info;

    SM_CONLOC *conloc;        
    long max_num_conloc;
    long num_conloc;

    PARSETYPE_FUNC parse_func[SM_PF_NUM_PF];
    int single_case;

    long para_num, sent_num;
} STATIC_INFO;

static STATIC_INFO *info;
static int max_inst = 0;

static int init_parsetype_func(), add_con();


int 
init_full_all_parse(spec, param_prefix)
SPEC *spec;
char *param_prefix;
{
    STATIC_INFO *ip;
    int new_inst;
    /* Lookup the values of the relevant parameters */
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: entering init_full_all_parse");

    NEW_INST (new_inst);
    if (UNDEF == new_inst)
        return (UNDEF);

    ip = &info[new_inst];


    /* For each of word, proper, number find the parsetype_func associated
       with that type of token, and initialize that function */
    if (UNDEF == init_parsetype_func ("word",
                                       spec,
                                       param_prefix,
                                       &ip->parse_func[SM_PT_F_WORD]) ||
        UNDEF == init_parsetype_func ("proper",
                                       spec,
                                       param_prefix,
                                       &ip->parse_func[SM_PT_F_PROPER]) ||
        UNDEF == init_parsetype_func ("number",
                                       spec,
                                       param_prefix,
                                       &ip->parse_func[SM_PT_F_NUMBER]))
        return (UNDEF);

    ip->max_num_conloc = 0;
    ip->single_case = single_case_flag;

    ip->valid_info = 1;

    PRINT_TRACE (2, print_string, "Trace: leaving init_full_all_parse");

    return (new_inst);
}

/* WARNING: for efficiency, full_all_parse changes the input token IN PLACE */

int
full_all_parse (tokensect, consect, inst)
SM_TOKENSECT *tokensect;          /* Input tokens */
SM_CONSECT *consect;
int inst;
{
    STATIC_INFO *ip;
    SM_TOKENTYPE *token;
    char *token_ptr;
    int first_capital, other_capital;
    int last_word_ended_sent = 1;
    long i;

    PRINT_TRACE (2, print_string, "Trace: entering full_all_parse");
    PRINT_TRACE (6, print_tokensect, tokensect);

    if (! VALID_INST (inst)) {
        set_error (SM_ILLPA_ERR, "Instantiation", "full_all_parse");
        return (UNDEF);
    }
    ip  = &info[inst];

    /* Reserve space for conlocs.  In full_all_parse (as opposed to some
       phrasing methods) we are guaranteed to have less conlocs than
       tokens */
    if (tokensect->num_tokens > ip->max_num_conloc) {
        if (ip->max_num_conloc > 0)
            (void) free ((char *) ip->conloc);
        ip->max_num_conloc += tokensect->num_tokens;
        if (NULL == (ip->conloc = (SM_CONLOC *)
                     malloc ((unsigned) ip->max_num_conloc
                             * sizeof (SM_CONLOC))))
            return (UNDEF);
    }

    /* Initialize conloc info */
    ip->para_num = ip->sent_num = 0;
    ip->num_conloc = 0;

    for (i = 0; i < tokensect->num_tokens; i++) {
        token = &tokensect->tokens[i];
        switch (token->tokentype) {
          case SM_INDEX_LOWER:
            if (UNDEF == add_con (ip,
                                  token->token,
                                  &ip->parse_func[SM_PT_F_WORD],
                                  i))
                return (UNDEF);
            last_word_ended_sent = 0;
            break;
          case SM_INDEX_MIXED:
            token_ptr = token->token;
            first_capital = 0;
            other_capital = 0;
            if (! islower (*token_ptr)) {
                if (isupper (*token_ptr))
                    *token_ptr = tolower (*token_ptr);
                first_capital = 1;
                token_ptr++;
            }
            while (*token_ptr) {
                if (! islower (*token_ptr)) {
                    if (isupper (*token_ptr))
                        *token_ptr = tolower (*token_ptr);
                    other_capital = 1;
                }
                token_ptr++;
            }
            /* Check to see if capitalized because start of sentence */
            /* or because all single case (therefore no proper nouns) */
            if (ip->single_case ||
                (first_capital && other_capital == 0 &&
                 last_word_ended_sent &&
                 (token->token[1] != '\0' ||
                  token->token[0] == 'a' ||
                  token->token[0] == 'i'))) {
                /* More than one letter (or A or I), only first
                   capitalized, at beginning of sentence */
                if (UNDEF == add_con (ip,
                                      token->token,
                                      &ip->parse_func[SM_PT_F_WORD],
                                      i))
                    return (UNDEF);
            }
            else {
                if (UNDEF == add_con (ip,
                                      token->token,
                                      &ip->parse_func[SM_PT_F_PROPER],
                                      i))
                    return (UNDEF);
            }
            last_word_ended_sent = 0;
            break;
          case SM_INDEX_DIGIT:
            if (UNDEF == add_con (ip,
                                  token->token,
                                  &ip->parse_func[SM_PT_F_NUMBER],
                                  i))
                return (UNDEF);
            last_word_ended_sent = 0;
            break;
          case SM_INDEX_SENT:
            last_word_ended_sent = 1;
            ip->sent_num++;
            break;
          case SM_INDEX_PARA:
            last_word_ended_sent = 1;
            ip->para_num++;
            ip->sent_num++;
            break;
          case SM_INDEX_PUNC:
            if (check_sentence (token, ip->single_case)) {
                last_word_ended_sent = 1;
                ip->sent_num++;
            }
            break;
          case SM_INDEX_NLSPACE:
            if (check_paragraph (token, last_word_ended_sent)) {
                last_word_ended_sent = 1;
                ip->sent_num++;
                ip->para_num++;
            }
            break;
          case SM_INDEX_SPACE:
          case SM_INDEX_IGNORE:
            break;
          case SM_INDEX_SECT_START:
          default:
            return (UNDEF);
        }
    }

    consect->concepts = ip->conloc;
    consect->num_concepts = ip->num_conloc;
    
    PRINT_TRACE (4, print_consect, consect);
    PRINT_TRACE (2, print_string, "Trace: leaving full_all_parse");

    return (1);
}

int 
close_full_all_parse(inst)
int inst;
{
    STATIC_INFO *ip;
    long i;

    PRINT_TRACE (2, print_string, "Trace: entering close_full_all_parse");

    if (! VALID_INST (inst)) {
        set_error (SM_ILLPA_ERR, "Instantiation", "close_full_all_parse");
        return (UNDEF);
    }

    ip  = &info[inst];
    ip->valid_info--;

    if (ip->valid_info == 0) {
        if (0 != ip->max_num_conloc)
            (void) free ((char *) ip->conloc);
        for (i = 0; i < SM_PF_NUM_PF; i++) {
            if (UNDEF == ip->parse_func[i].ctype)
                continue;
            if (UNDEF == ip->parse_func[i].stemmer.ptab->close_proc
                (ip->parse_func[i].stemmer.inst) ||
                UNDEF == ip->parse_func[i].stopword.ptab->close_proc
                (ip->parse_func[i].stopword.inst) ||
                UNDEF == ip->parse_func[i].token_to_con.ptab->close_proc
                (ip->parse_func[i].token_to_con.inst))
                return (UNDEF);
        }
    }

    PRINT_TRACE (2, print_string, "Trace: leaving close_full_all_parse");

    return (0);
}


/* This version of init_parsetype_func obtains all values from scratch (as
   opposed to obtaining from doc_desc).  It illustrates how to obtain these
   values in case there is some parsetype info desired that is not included
   in doc_desc. */
static int
init_parsetype_func (parsetype, spec, prefix, parse_func)
char *parsetype;
SPEC *spec;
char *prefix;
PARSETYPE_FUNC *parse_func;
{
    char buf[PATH_LEN];
    char *ptr = buf;
    char *end_buf;
    char *old_ptr = prefix;
    SPEC_PARAM spec_generic;

    while (*old_ptr)
        *ptr++ = *old_ptr++;
    old_ptr = parsetype;
    while (*old_ptr)
        *ptr++ = *old_ptr++;
    *ptr++ = '.';
    end_buf = ptr;

    spec_generic.param = buf;

    (void) strcpy (end_buf, "ctype");
    spec_generic.result = (char *) &parse_func->ctype;
    spec_generic.convert = getspec_long;
    if (UNDEF == lookup_spec (spec, &spec_generic, 1))
        return (UNDEF);
    if (parse_func->ctype == UNDEF)
        /* This type not indexed */
        return (0);

    (void) strcpy (end_buf, "stemmer");
    spec_generic.result = (char *) &parse_func->stemmer.ptab;
    spec_generic.convert = getspec_func;
    if (UNDEF == lookup_spec (spec, &spec_generic, 1))
        return (UNDEF);
    *end_buf = '\0';
    if (UNDEF == (parse_func->stemmer.inst = 
                  parse_func->stemmer.ptab->init_proc (spec, buf)))
        return (UNDEF);

    (void) strcpy (end_buf, "stopword");
    spec_generic.result = (char *) &parse_func->stopword.ptab;
    spec_generic.convert = getspec_func;
    if (UNDEF == lookup_spec (spec, &spec_generic, 1))
        return (UNDEF);
    *end_buf = '\0';
    if (UNDEF == (parse_func->stopword.inst =
                  parse_func->stopword.ptab->init_proc (spec, buf)))
        return (UNDEF);

    (void) strcpy (end_buf, "token_to_con");
    spec_generic.result = (char *) &parse_func->token_to_con.ptab;
    spec_generic.convert = getspec_func;
    if (UNDEF == lookup_spec (spec, &spec_generic, 1))
        return (UNDEF);
    *end_buf = '\0';
    if (UNDEF == (parse_func->token_to_con.inst =
                  parse_func->token_to_con.ptab->init_proc (spec, buf)))
        return (UNDEF);

    return (1);
}


/* Add token to the conloc info if it fits criteria.  Here, check to see
   if stopword, then stem, and then call token_to_con to get concept number */
static int
add_con (ip, token, parse_func, token_num)
STATIC_INFO *ip;
char *token;
PARSETYPE_FUNC *parse_func;
long token_num;
{
    long stop_value;
    char *new_stem;

    SM_CONLOC *conloc = &ip->conloc[ip->num_conloc];
    if (parse_func->ctype == UNDEF)
        return (0);

    /* Check to see if token is a stopword */
    if (UNDEF == parse_func->stopword.ptab->proc (token,
                                                  &stop_value,
                                                  parse_func->stopword.inst))
        return (UNDEF);
    if (1 == stop_value)
        return (0);

    /* Stem (Note that this can be destructive and done in place) */
    if (UNDEF == parse_func->stemmer.ptab->proc (token,
                                                 &new_stem,
                                                 parse_func->stemmer.inst))
        return (UNDEF);
    
    /* Convert to a concept number */
    if (UNDEF == parse_func->token_to_con.ptab->proc
                 (new_stem, &conloc->con, parse_func->token_to_con.inst))
        return (UNDEF);

    if (UNDEF == conloc->con)
        return (0);

    conloc->ctype = parse_func->ctype;
    conloc->para_num = ip->para_num;
    conloc->sent_num = ip->sent_num;
    conloc->token_num = token_num;

    ip->num_conloc++;
    return (1);
}
