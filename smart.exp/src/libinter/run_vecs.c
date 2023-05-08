#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libinter/run_vecs.c,v 11.0 1992/07/21 18:21:46 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

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

static PROC_INST p_retrieve;
static PROC_INST p_feedback;
static PROC_INST user_qdisp;
static PROC_INST qdisp_query;

static SPEC_PARAM spec_args[] = {
    "inter.coll_sim",            getspec_func,    (char *) &p_retrieve.ptab,
    "inter.feedback",            getspec_func,    (char *) &p_feedback.ptab,
    "inter.user_qdisp",          getspec_func,    (char *) &user_qdisp.ptab,
    "inter.qdisp_query",         getspec_func,    (char *) &qdisp_query.ptab,
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static int num_inst = 0;

static int compare_tr_did();  /* Sorts tr_vec by doc id */

int inter_prepare_titles(), show_titles();

int
init_run_vec (spec, unused)
SPEC *spec;
char *unused;
{
    if (num_inst++ > 0) {
        return (num_inst);
    }
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    /* Initialize other procedures */
    if (UNDEF == (p_retrieve.inst = p_retrieve.ptab->init_proc (spec, NULL)) ||
        UNDEF == (p_feedback.inst = p_feedback.ptab->init_proc (spec, NULL)) ||
        UNDEF == (user_qdisp.inst = user_qdisp.ptab->init_proc (spec, NULL)) ||
        UNDEF == (qdisp_query.inst = qdisp_query.ptab->init_proc (spec, NULL)))
        return (UNDEF);

    return (num_inst);
}


int
close_run_vec (inst)
int inst;
{
    if (--num_inst > 0)
        return (0);

    if (UNDEF == p_retrieve.ptab->close_proc (p_retrieve.inst) ||
        UNDEF == p_feedback.ptab->close_proc (p_feedback.inst) ||
        UNDEF == user_qdisp.ptab->close_proc (user_qdisp.inst) ||
        UNDEF == qdisp_query.ptab->close_proc (qdisp_query.inst))
        return (UNDEF);

    return (1);
}

/* Run a retrieval with query is->retrieval.query, and seen_docs
   is->retrieval.tr, adding results to is->retrieval.tr */
static int
run_retrieve (is)
INTER_STATE *is;
{
    RESULT results;          /* Full results of current query */

    /* Restore tr_vec to be sorted in increasing did order */
    if (is->retrieval.tr->num_tr > 0)
        qsort ((char *) is->retrieval.tr->tr,
               (int) is->retrieval.tr->num_tr,
               sizeof (TR_TUP), compare_tr_did);

    /* Save the query vector just to make sure doesn't get overwritten */
    /* SHOULDN'T NEED */
/*    if (UNDEF == save_vec ((VEC *) is->retrieval.query->vector)) */
/*        return (UNDEF);*/

    if (UNDEF == p_retrieve.ptab->proc (&is->retrieval,
                                        &results,
                                        p_retrieve.inst))
        return (UNDEF);
    if (UNDEF == res_tr (&results, is->retrieval.tr))
        return (UNDEF);

    if (UNDEF == inter_prepare_titles (is, (char *) NULL))
        return (UNDEF);

    is->current_doc = 0;

    if (is->verbose_level)
        return (show_titles (is, (char *) NULL));

    return (1);
}

int
run_qvec (is, unused)
INTER_STATE *is;
char *unused;
{
    if (is->retrieval.query->vector == NULL) {
        if (UNDEF == add_buf_string ("No valid query.  Command ignored\n",
                                     &is->err_buf))
            return (UNDEF);
        return (0);
    }

    return (run_retrieve (is));
}

static VEC qvec;

int
run_dvec (is, unused)
INTER_STATE *is;
char *unused;
{
    VEC_LIST vec_list;
    VEC temp_vec;

    vec_list.vec = &temp_vec;

    if (is->num_command_line < 2 ||
        UNDEF == inter_get_sect_veclist (is->command_line[1], &vec_list) ||
        vec_list.num_vec != 1) {
        if (UNDEF == add_buf_string (
                              "Not a valid single document. Command ignored\n",
                               &is->err_buf))
            return (UNDEF);
        return (0);
    }

    /* MEMORY LEAK */
    if (UNDEF == copy_vec (&qvec, &vec_list.vec[0]))
        return (UNDEF);

    is->retrieval.tr->num_tr = 0;
    is->retrieval.query->vector = (char *) &qvec;
    is->retrieval.query->qid = qvec.id_num;

    return (run_retrieve (is));
}


int
run_feedback(is, unused)
INTER_STATE *is;
char *unused;
{
    QUERY_VECTOR fdbk_query;
    TR_VEC *tr_vec;
    long command, i;
    long docid, sectid, paraid, sentid;
    char temp_buf[PATH_LEN];

    if (is->retrieval.query->vector == (char *) NULL) {
        if (UNDEF == add_buf_string ("No query to run feedback on.",
                                     &is->err_buf))
            return (UNDEF);
        return (0);
    }

    /* Docids on command line are relevant */
    tr_vec = is->retrieval.tr;
    for (command = 1; command < is->num_command_line; command++) {
        if (UNDEF == inter_get_docid (is->command_line[command],
                                      &docid, &sectid, &paraid, &sentid))
            return (UNDEF);

        for (i = 0;
             i < tr_vec->num_tr && docid != tr_vec->tr[i].did;
             i++)
            ;
        if (i >= tr_vec->num_tr) {
            (void) sprintf (temp_buf,
                            "Document %ld cannot be used for feedback.",docid);
            if (UNDEF == add_buf_string (temp_buf, &is->err_buf))
                return (UNDEF);
            return (0);
        }
        tr_vec->tr[i].rel = 1;
    }
    
    /* Restore tr_vec to be sorted in increasing did order */
    if (is->retrieval.tr->num_tr > 0)
        qsort ((char *) is->retrieval.tr->tr,
               (int) is->retrieval.tr->num_tr,
               sizeof (TR_TUP), compare_tr_did);

    /* Create feedback query */
    if (UNDEF == p_feedback.ptab->proc (&is->retrieval,
                                        &fdbk_query,
                                        p_feedback.inst))
        return (UNDEF);
    if (fdbk_query.vector != NULL)
        is->retrieval.query->vector = fdbk_query.vector;
    
    return (run_retrieve(is));
}


int
run_new (is, unused)
INTER_STATE *is;
char *unused;
{
    int status;
    QUERY_VECTOR query_vec;

    if (1 != (status = user_qdisp.ptab->proc ((char *) NULL,
                                              &is->query_textloc,
                                              user_qdisp.inst))) {
        return (status);
    }
    if (1 != (status = qdisp_query.ptab->proc (&is->query_textloc,
                                               &query_vec,
                                               qdisp_query.inst))) {
        return (status);
    }

    is->retrieval.tr->num_tr = 0;
    is->retrieval.query->vector = query_vec.vector;
    is->retrieval.query->qid = query_vec.qid;

    return (run_retrieve (is));
}

static int
compare_tr_did (tr1, tr2)
register TR_TUP *tr1;
register TR_TUP *tr2;
{
    if (tr1->did < tr2->did) {
        return (-1);
    }
    if (tr1->did > tr2->did) {
        return (1);
    }
    return (0);
}

