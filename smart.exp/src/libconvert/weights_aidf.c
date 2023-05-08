#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libconvert/weights_aidf.c,v 11.0 1992/07/21 18:20:27 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/********************   PROCEDURE DESCRIPTION   ************************
 *0 Reweight phrase vector by multiplying weight by average of term idf factors
 *1 convert.wt_idf.A
 *2 idfwt_avphrase (vec, unused, inst)
 *3   VEC *vec;
 *3   char *unused;
 *3   int inst;
 *4 init_idfwt_avphrase (spec, param_prefix)
 *5   "convert.weight.trace"
 *5   "doc.textloc_file"
 *5   "*.inv_file"
 *5   "*.inv_file.rmode"
 *4 close_idfwt_avphrase (inst)

 *7 For each term in vector (assumed to be a phrase), multiply its 
 *7 existing weight by the average of log (N / ni) for its component terms.
 *7 N is the number of docs in the collection, determined from the number
 *7 of entries in "textloc_file".  ni is the number of docs in which each term
 *7 i occurs, determined by the number of entries in the inverted list for
 *7 the term (from "inv_file").
 *7 Note that "inv_file" is a ctype dependant parameter, and is found by
 *7 looking up the value of the parameter formed by concatenating 
 *7 param_prefix and "inv_file".
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
#include "inv.h"
#include "inst.h"

static long major_ctype;

static SPEC_PARAM spec_args[] = {
    "convert.weight.major_ctype",  getspec_long,  (char *) &major_ctype,
    TRACE_PARAM ("convert.weight.trace")
    };
static int num_spec_args = sizeof (spec_args) / sizeof (spec_args[0]);

/* Static info to be kept for each instantiation of this proc */
typedef struct {
    /* bookkeeping */
    int valid_info;

    PROC_INST phrase_proc;
    PROC_INST term_proc;
    long major_ctype;
    int idf_inst;            /*Instantiation of coll info procedure */
} STATIC_INFO;

static STATIC_INFO *info;
static int max_inst = 0;

int
init_idfwt_avphrase (spec, param_prefix)
SPEC *spec;
char *param_prefix;
{
    STATIC_INFO *ip;
    int new_inst;
    SPEC_PARAM spec_param;
    char param[PATH_LEN];

    /* Lookup the values of the relevant parameters */
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args)) {
        return (UNDEF);
    }
    PRINT_TRACE (2, print_string, "Trace: entering init_idfwt_idf");

    NEW_INST (new_inst);
    if (UNDEF == new_inst)
        return (UNDEF);

    ip = &info[new_inst];


    /* Initialize procedure to convert phrase concept to token */
    if (param_prefix != NULL)
        (void) strcpy (param, param_prefix);
    else
        param[0] = '\0';
    (void) strcat (param, "con_to_token");
    spec_param.param = param;
    spec_param.convert = getspec_func;
    spec_param.result = (char *) &ip->phrase_proc.ptab;
    if (UNDEF == lookup_spec (spec, &spec_param, 1))
        return (UNDEF);
    if (UNDEF == (ip->phrase_proc.inst =
                  ip->phrase_proc.ptab->init_proc (spec, param_prefix)))
        return (UNDEF);

    /* Initialize procedure to convert term token to concept */
    (void) sprintf (param, "ctype.%ld.token_to_con", major_ctype);
    spec_param.param = param;
    spec_param.convert = getspec_func;
    spec_param.result = (char *) &ip->term_proc.ptab;
    if (UNDEF == lookup_spec (spec, &spec_param, 1))
        return (UNDEF);
    (void) sprintf (param, "ctype.%ld.", major_ctype);
    if (UNDEF == (ip->term_proc.inst =
                  ip->term_proc.ptab->init_proc (spec, param)))
        return (UNDEF);

    /* Initialize getting idf value for the terms */
    if (UNDEF == (ip->idf_inst = init_con_cw_idf (spec, param)))
        return (UNDEF);

    ip->major_ctype = major_ctype;
    ip->valid_info = 1;

    PRINT_TRACE (2, print_string, "Trace: leaving init_idfwt_avphrase");
    return (new_inst);
}


/* Weight phrase term by the average of the idf of the included single terms */
/* Assumptions: All individual components of phrase (ie, anything separated 
   by white-space) occur in the term dictionary as well as the inverted file.
*/

int
idfwt_avphrase (vec, unused, inst)
VEC *vec;
char *unused;
int inst;
{
    long i;
    CON_WT *conwt = vec->con_wtp;
    char save_token[MAX_TOKEN_LEN];
    STATIC_INFO *ip;
    float idf;
    float sum_idf;
    long num_idf;
    long con;

    char *ptr, *start_ptr, *end_ptr;
    char *phrase_token;

    if (! VALID_INST (inst)) {
        set_error (SM_ILLPA_ERR, "Instantiation", "idfwt_avphrase");
        return (UNDEF);
    }
    ip  = &info[inst];
    for (i = 0; i < vec->num_conwt; i++) {
        sum_idf = 0.0;
        num_idf = 0;
        /* Find original phrase from dictionary */
        if (1 != ip->phrase_proc.ptab->proc (&conwt->con,
                                             &phrase_token,
                                             ip->phrase_proc.inst)) {
            set_error (SM_INCON_ERR, "Phrase not in dict", "idfwt_avphrase");
            return (UNDEF);
        }
        (void) strncpy (save_token, phrase_token, MAX_TOKEN_LEN);
        end_ptr = &save_token[strlen(save_token)];
        ptr = start_ptr = save_token;
        while (ptr <= end_ptr) {
            if (*ptr == '\0' || isspace (*ptr)) {
                /* Found word boundary; lookup word in dict (if non-empty) */
                if (ptr > start_ptr) {
                    *ptr = '\0';
                    if (1 != ip->term_proc.ptab->proc (start_ptr,
                                                       &con,
                                                       ip->term_proc.inst)) {
                        set_error (SM_INCON_ERR,
                                   "Phrase component not in dict",
                                   "idfwt_avphrase");
                        return (UNDEF);
                    }
                    if (UNDEF == con_cw_idf (&con, &idf, ip->idf_inst))
                        return (UNDEF);
                    sum_idf += idf;
                    num_idf++;
                }
                start_ptr = ptr+1;
            }
            ptr++;
        }

        if (num_idf == 0) {
            set_error (SM_INCON_ERR,
                       "Phrase components have no idf",
                       "idfwt_avphrase");
            return (UNDEF);
        }
        conwt->wt *= sum_idf / num_idf;
        conwt++;
    }
    return (1);
}


int
close_idfwt_avphrase (inst)
int inst;
{
    STATIC_INFO *ip;
    PRINT_TRACE (2, print_string, "Trace: entering close_idfwt_avphrase");

    if (! VALID_INST (inst)) {
        set_error (SM_ILLPA_ERR, "Instantiation", "close_idfwt_avphrase");
        return (UNDEF);
    }
    ip  = &info[inst];
    ip->valid_info--;

    if (UNDEF == ip->phrase_proc.ptab->close_proc (ip->phrase_proc.inst) ||
        UNDEF == ip->term_proc.ptab->close_proc (ip->term_proc.inst) ||
        UNDEF == close_con_cw_idf (ip->idf_inst))
        return (UNDEF);

    PRINT_TRACE (2, print_string, "Trace: leaving close_idfwt_avphrase");

    return (0);
}
