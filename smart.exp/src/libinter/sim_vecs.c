#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libinter/sim_vecs.c,v 11.0 1992/07/21 18:21:53 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "io.h"
#include "spec.h"
#include "proc.h"
#include "tr_vec.h"
#include "retrieve.h"
#include "inter.h"

static PROC_INST sect_retrieve;      /* Retrieval method for sections */
static long sect_num_wanted;         /* number of results wanted when, eg,
                                        comparing sentences vs sentences */

static SPEC_PARAM spec_args[] = {
    "inter.vecs_vecs",           getspec_func,    (char *) &sect_retrieve.ptab,
    "sect.num_wanted",           getspec_long,    (char *) &sect_num_wanted,
};
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static int num_inst = 0;

static int compare_res_sim();

int
init_sim_vec (spec, unused)
SPEC *spec;
char *unused;
{
    if (num_inst++ > 0) {
        return (num_inst);
    }
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    if (UNDEF == (sect_retrieve.inst =
                  sect_retrieve.ptab->init_proc (spec, NULL)))
        return (UNDEF);

    return (num_inst);
}


int
close_sim_vec (inst)
int inst;
{
    if (--num_inst > 0)
        return (0);

    if (UNDEF == sect_retrieve.ptab->close_proc (sect_retrieve.inst))
        return (UNDEF);

    return (1);
}

/* Given two vec_lists (one or both possibly consisting of single vector)
   compute the similarity of each member of the first list against each
   of the second list and report the results
*/
static int
sim_retrieve (is, vec_list1, vec_list2)
INTER_STATE *is;
VEC_LIST *vec_list1;
VEC_LIST *vec_list2;
{
    VEC_LIST_PAIR vec_list_pair;
    ALT_RESULTS sim_results;
    long i;
    char temp_buf[PATH_LEN];
    
    vec_list_pair.vec_list1 = vec_list1;
    vec_list_pair.vec_list2 = vec_list2;

    if (UNDEF == sect_retrieve.ptab->proc (&vec_list_pair,
                                           &sim_results,
                                           sect_retrieve.inst))
        return (UNDEF);

    if (sim_results.num_results == 0) {
        if (UNDEF == add_buf_string ("All similarities computed were 0.0",
                                     &is->output_buf))
            return (UNDEF);
        return (1);
    }
    /* Sort sim_results by decreasing sim */
    qsort ((char *) sim_results.results,
           (int) sim_results.num_results,
           sizeof (RESULT_TUPLE),
           compare_res_sim);

    for (i = 0; i < MIN (sect_num_wanted, sim_results.num_results); i++) {
        (void) sprintf (temp_buf,
                        "%ld\t%ld\t%6.4f\n",
                        sim_results.results[i].qid,
                        sim_results.results[i].did,
                        sim_results.results[i].sim);
        if (UNDEF == add_buf_string (temp_buf, &is->output_buf))
            return (UNDEF);
    }

    return (1);
}

int
sim_qvec (is, unused)
INTER_STATE *is;
char *unused;
{
    VEC_LIST vec_list1, vec_list2;
    VEC vec[2];

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
    vec_list1.vec = (VEC *) is->retrieval.query->vector;
    vec_list1.num_vec = 1;
    vec_list2.vec = &vec[0];

    if (UNDEF == inter_get_sect_veclist (is->command_line[1], &vec_list2)) {
        if (UNDEF == add_buf_string ("Cannot index doc", &is->err_buf))
            return (UNDEF);
        return (0);
    }

    return (sim_retrieve (is, &vec_list1, &vec_list2));
}


int
sim_dvec (is, unused)
INTER_STATE *is;
char *unused;
{
    int status;
    VEC_LIST vec_list1, vec_list2;
    VEC vec1[2], vec2[2];

    if (is->num_command_line < 3) {
        if (UNDEF == add_buf_string ("Must specify two docs", &is->err_buf))
            return (UNDEF);
        return (0);
    }

    vec_list1.vec = &vec1[0];
    vec_list2.vec = &vec2[0];

    if (UNDEF == inter_get_sect_veclist (is->command_line[1], &vec_list1)) {
        if (UNDEF == add_buf_string ("Cannot index first doc", &is->err_buf))
            return (UNDEF);
        return (0);
    }
    if (UNDEF == save_vec_list (&vec_list1))
        return (UNDEF);
    if (UNDEF == inter_get_sect_veclist (is->command_line[2], &vec_list2)) {
        if (UNDEF == add_buf_string ("Cannot index second doc", &is->err_buf))
            return (UNDEF);
        return (0);
    }

    status = sim_retrieve (is, &vec_list1, &vec_list2);

    if (UNDEF == free_vec_list (&vec_list1))
        return (UNDEF);

    return (status);
}


static int
compare_res_sim (ptr1, ptr2)
RESULT_TUPLE *ptr1, *ptr2;
{
    if (ptr1->sim > ptr2->sim)
        return (-1);
    if (ptr1->sim < ptr2->sim)
        return (1);
    return (0);
}
