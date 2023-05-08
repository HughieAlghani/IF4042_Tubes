#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libindexing/parse_asis.c,v 11.0 1992/07/21 18:21:17 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 parse a tokenized text section composed of already parsed input
 *1 index.parse_sect.asis
 *2 asis_parse (token, consect, inst)
 *3   SM_TOKENSECT *token;
 *3   SM_CONSECT *consect;
 *3   inst;
 *4 init_asis_parse(spec, unused)
 *5   "index.parse_sect.trace"
 *4 close_asis_parse(inst)
 *7 Each line is an already parsed entry, of the form
 *7    token_num  ctype  docid
 *7 Just convert token_num to a concept number.
***********************************************************************/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "smart_error.h"
#include "proc.h"
#include "spec.h"
#include "docindex.h"
#include "trace.h"
#include "inst.h"

static SPEC_PARAM spec_args[] = {
    TRACE_PARAM ("index.parse_sect.trace")
    };
static int num_spec_args = sizeof (spec_args) / sizeof (spec_args[0]);

/* Static info to be kept for each instantiation of this proc */
typedef struct {
    /* bookkeeping */
    int valid_info;

    SM_CONLOC *conloc;        
    unsigned max_num_conloc;
    long num_conloc;
} STATIC_INFO;

static STATIC_INFO *info;
static int max_inst = 0;

int 
init_asis_parse(spec, unused)
SPEC *spec;
char *unused;
{
    STATIC_INFO *ip;
    int new_inst;
    /* Lookup the values of the relevant parameters */
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args)) {
        return (UNDEF);
    }

    PRINT_TRACE (2, print_string, "Trace: entering init_asis_parse");
    NEW_INST (new_inst);
    if (UNDEF == new_inst)
        return (UNDEF);

    ip = &info[new_inst];
    ip->max_num_conloc = 0;
    ip->valid_info = 1;

    PRINT_TRACE (2, print_string, "Trace: leaving init_asis_parse");

    return (new_inst);
}

int
asis_parse (tokensect, consect, inst)
SM_TOKENSECT *tokensect;          /* Input tokens */
SM_CONSECT *consect;
int inst;
{
    STATIC_INFO *ip;
    int current_field = 0;       /* Of the three fields of each line, the
                                    one expected next (0, 1, 2) */
    SM_TOKENTYPE *token;         /* current token under consideration */
    SM_CONLOC *conloc;           /* current conloc entry being filled */
    char *ptr;
    long i;

    PRINT_TRACE (2, print_string, "Trace: entering asis_parse");
    PRINT_TRACE (6, print_tokensect, tokensect);

    if (! VALID_INST (inst)) {
        set_error (SM_ILLPA_ERR, "Instantiation", "full_parse");
        return (UNDEF);
    }
    ip  = &info[inst];

    /* Reserve space for conlocs.  In asis_parse (as opposed to some
       phrasing methods) we are guaranteed to have less conlocs than
       tokens */
    if (tokensect->num_tokens > ip->max_num_conloc) {
        if (ip->max_num_conloc > 0)
            (void) free ((char *) ip->conloc);
        ip->max_num_conloc = tokensect->num_tokens;
        if (NULL == (ip->conloc = (SM_CONLOC *)
                     malloc (ip->max_num_conloc * sizeof (SM_CONLOC))))
            return (UNDEF);
    }

    conloc = ip->conloc;

    for (i = 0; i < tokensect->num_tokens; i++) {
        token = &tokensect->tokens[i];
        if (token->tokentype == SM_INDEX_DIGIT) {
            if (current_field == 0) {
                current_field = 1;
                conloc->con = atol (token->token);
            }
            else if (current_field == 1) {
                conloc->ctype = atol (token->token);
                current_field = 2;
            }
            else {
                /* Ignore third field (the did), but can now return
                   the token */
                conloc->para_num = 0;
                conloc->sent_num = i;
                conloc->token_num = i;
                conloc++;
            }
        }
        else if (token->tokentype == SM_INDEX_SPACE && current_field == 2) {
            /* Check for end of line */
            for (ptr = token->token; *ptr; ptr++) {
                if (*ptr == '\n')
                    current_field = 0;
            }
        }
    }

    consect->concepts = ip->conloc;
    consect->num_concepts = conloc - ip->conloc;

    PRINT_TRACE (4, print_consect, consect);
    PRINT_TRACE (2, print_string, "Trace: leaving asis_parse");

    return (1);
}

int 
close_asis_parse(inst)
int inst;
{
    STATIC_INFO *ip;

    PRINT_TRACE (2, print_string, "Trace: entering close_asis_parse");

    if (! VALID_INST (inst)) {
        set_error (SM_ILLPA_ERR, "Instantiation", "close_asis_parse");
        return (UNDEF);
    }

    ip  = &info[inst];
    ip->valid_info--;

    if (ip->valid_info == 0) {
        if (0 != ip->max_num_conloc)
            (void) free ((char *) ip->conloc);
    }

    PRINT_TRACE (2, print_string, "Trace: leaving close_asis_parse");
    return (0);
}
