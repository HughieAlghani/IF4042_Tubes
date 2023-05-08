#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libinter/match_vecs.c,v 11.0 1992/07/21 18:21:46 chrisb Exp $";
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
#include "textloc.h"
#include "io.h"
#include "smart_error.h"
#include "spec.h"
#include "proc.h"
#include "sm_display.h"
#include "tr_vec.h"
#include "context.h"
#include "retrieve.h"
#include "docindex.h"
#include "inter.h"

static SM_INDEX_DOCDESC doc_desc;
static int *contok_inst;

static int num_inst = 0;

static int get_vec();

int
init_match_vec (spec, unused)
SPEC *spec;
char *unused;
{
    char param_prefix[PATH_LEN];
    long i;

    if (num_inst++ > 0) {
        return (num_inst);
    }
    if (UNDEF == lookup_spec_docdesc (spec, &doc_desc))
        return (UNDEF);

    /* Reserve space for the instantiation ids of the called procedures. */
    if (NULL == (contok_inst = (int *)
                 malloc ((unsigned) doc_desc.num_ctypes * sizeof (int))))
        return (UNDEF);

     for (i = 0; i < doc_desc.num_ctypes; i++) {
        /* Set param_prefix to be current parameter path for this ctype.
           This will then be used by the con_to_token routine to lookup
           parameters it needs. */
        (void) sprintf (param_prefix, "index.ctype.%ld.", i);
        if (UNDEF == (contok_inst[i] =
                      doc_desc.ctypes[i].con_to_token->init_proc
                      (spec, param_prefix)))
            return (UNDEF);
    }

    return (num_inst);
}


int
close_match_vec (inst)
int inst;
{
    long i;

    if (--num_inst > 0)
        return (0);

    for (i = 0; i < doc_desc.num_ctypes; i++) {
        if (UNDEF == doc_desc.ctypes[i].con_to_token->
            close_proc(contok_inst[i]))
            return (UNDEF);
    }

    (void) free ((char *) contok_inst);

    return (1);
}

static int
match_vec (is, vec1, vec2)
INTER_STATE *is;
VEC *vec1, *vec2;

{
    long ctype;                /* Current ctype checking for match */
    CON_WT *conwt1, *conwt2;   /* Current concepts checking */
    CON_WT *end_ctype1, *end_ctype2; /* Last concept for this ctype */
    char temp_buf[PATH_LEN];
    char *token;
    int match_flag = 0;

    if (UNDEF == add_buf_string ("Ctype\tCon\tVec1\tVec2\t\tProduct\tToken\n",
                                 &is->output_buf))
        return (UNDEF);
    conwt1 = vec1->con_wtp;
    conwt2 = vec2->con_wtp;
    for (ctype = 0;
         ctype < vec1->num_ctype && ctype < vec2->num_ctype; 
         ctype++) {
        end_ctype1 = &conwt1[vec1->ctype_len[ctype]];
        end_ctype2 = &conwt2[vec2->ctype_len[ctype]];
        while (conwt1 < end_ctype1 && conwt2 < end_ctype2) {
            if (conwt1->con < conwt2->con)
                conwt1++;
            else if (conwt1->con > conwt2->con)
                conwt2++;
            else {
                /* have match */
                if (UNDEF == doc_desc.ctypes[ctype].con_to_token->
                    proc (&conwt1->con, &token, contok_inst[ctype])) {
                    token = "Not in dictionary";
                    clr_err();
                }
               (void) sprintf (temp_buf,
                                "%ld\t%ld\t%6.4f\t%6.4f\t\t%6.4f\t%s\n",
                                ctype, conwt1->con, conwt1->wt, conwt2->wt,
                                conwt1->wt * conwt2->wt,
                                token);
                if (UNDEF == add_buf_string (temp_buf, &is->output_buf))
                    return (UNDEF);
                conwt1++; conwt2++;
                match_flag++;
            }
        }
        conwt1 = end_ctype1;
        conwt2 = end_ctype2;
    }

    if (match_flag == 0)
        if (UNDEF == add_buf_string ("No matching concepts",&is->output_buf))
            return (UNDEF);

    return (1);
}


int
match_qvec (is, unused)
INTER_STATE *is;
char *unused;
{
    int status;
    VEC vec2;

    if (is->num_command_line < 2) {
        if (UNDEF == add_buf_string ("Must specify a doc", &is->err_buf))
            return (UNDEF);
        return (0);
    }
    if (NULL == is->retrieval.query->vector) {
        if (UNDEF == add_buf_string ("No query specified", &is->err_buf))
            return (UNDEF);
        return (0);
    }

    if (1 != (status = get_vec (is, is->command_line[1], &vec2)))
        return (status);

    return (match_vec (is, (VEC *) is->retrieval.query->vector, &vec2));

}


int
match_dvec (is, unused)
INTER_STATE *is;
char *unused;
{
    int status;
    VEC vec1, vec2;

    if (is->num_command_line < 3) {
        if (UNDEF == add_buf_string ("Must specify two docs", &is->err_buf))
            return (UNDEF);
        return (0);
    }

    if (1 != (status = get_vec (is,
                                is->command_line[1],
                                &vec1)))
        return (status);
    if (UNDEF == save_vec (&vec1))
        return (UNDEF);
    if (1 != (status = get_vec (is, 
                                is->command_line[2],
                                &vec2)))
        return (status);

    status = match_vec (is, &vec1, &vec2);
    
    if (UNDEF == free_vec (&vec1))
        return (UNDEF);

    return (status);
}


static int
get_vec (is, command_line, vec)
INTER_STATE *is;
char *command_line;
VEC *vec;
{
    VEC_LIST vec_list;
    VEC temp_vec;

    vec_list.vec = &temp_vec;

    if (UNDEF == inter_get_sect_veclist (command_line, &vec_list) ||
        vec_list.num_vec != 1) {
        if (UNDEF == add_buf_string ("Not a valid single document. Command ignored\n",
                                     &is->err_buf))
            return (UNDEF);
        return (0);
    }

    /* MEMORY LEAK */
    if (UNDEF == copy_vec (vec, &vec_list.vec[0]))
        return (UNDEF);

    return (1);
}
