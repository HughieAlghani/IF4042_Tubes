#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/user_contrib/smartx/libinter/run_vecs.c,v 11.0 1992/07/21 18:19:18 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

/************************************************************************
*                                                                       *
* 	For the X interface, x_run_new() was added.                     *
*                                                                       *
*    by Tong Li, graduate student in CS from Oregon State University,   *
*                e-mail: lit@mist.cs.orst.edu                           *
*                                                                       *
*       Victuria Chiu, senior in CS from Univ. of Cal. at Berkeley,     *
*                      e-mail: victoria@csr.lbl.gov                     *
*                                                                       *
*    at Lawrence berkeley Laboratory, summer 1991                       *
*                                                                       *
************************************************************************/

#include <ctype.h>
#include "common.h"
#include "param.h"
#include "functions.h"
#include "display.h"
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

static PROC_INST para_pre;           /* Paragraph preparser */
static PROC_INST sent_pre;           /* Sentence preparser */
static PROC_INST p_retrieve;
static PROC_INST p_feedback;
static PROC_INST sect_retrieve;      /* Retrieval method for sections */

static SPEC_PARAM spec_args[] = {
    "inter.para.preparse",       getspec_func,    (char *) &para_pre.ptab,
    "inter.sent.preparse",       getspec_func,    (char *) &sent_pre.ptab,
    "inter.coll_sim",            getspec_func,    (char *) &p_retrieve.ptab,
    "inter.feedback",            getspec_func,    (char *) &p_feedback.ptab,
    "inter.vecs_vecs",           getspec_func,    (char *) &sect_retrieve.ptab,
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static int num_inst = 0;

static int run_pvec(), run_svec(), run_sect_vec();
static int compare_tr_did();  /* Sorts tr_vec by doc id */

static int pp_veclist_inst;
static int did_vec_inst;
static int get_q_user_inst;
static int get_q_text_inst;
int inter_prepare_titles(), show_titles();

int
init_run_vec (spec, unused)
SPEC *spec;
char *unused;
{
    long old_context;

    if (num_inst++ > 0) {
        return (num_inst);
    }
    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    old_context = get_context();
    set_context (CTXT_DOC);
    if (UNDEF == (para_pre.inst = para_pre.ptab->init_proc (spec, NULL)) ||
        UNDEF == (sent_pre.inst = sent_pre.ptab->init_proc (spec, NULL)) ||
        UNDEF == (sect_retrieve.inst =
                  sect_retrieve.ptab->init_proc (spec, NULL)))
        return (UNDEF);

    if (UNDEF == (pp_veclist_inst = init_pp_veclist (spec, (char *) NULL)))
        return (UNDEF);
    if (UNDEF == (did_vec_inst = init_did_vec (spec, (char *) NULL)))
        return (UNDEF);
    set_context (old_context);

    /* Initialize other procedures */
    if (UNDEF == (p_retrieve.inst = p_retrieve.ptab->init_proc (spec, NULL)) ||
        UNDEF == (p_feedback.inst = p_feedback.ptab->init_proc (spec, NULL)))
        return (UNDEF);
    if (UNDEF == (get_q_user_inst = init_get_q_user (spec, (char *) NULL)) ||
        UNDEF == (get_q_text_inst = init_get_q_text (spec, (char *) NULL)))
        return (UNDEF);

    return (num_inst);
}


int
close_run_vec (inst)
int inst;
{
    if (--num_inst > 0)
        return (0);

    if (UNDEF == para_pre.ptab->close_proc (para_pre.inst) ||
        UNDEF == sent_pre.ptab->close_proc (sent_pre.inst) ||
        UNDEF == close_pp_veclist (pp_veclist_inst) ||
        UNDEF == close_did_vec (did_vec_inst) ||
        UNDEF == p_retrieve.ptab->close_proc (p_retrieve.inst) ||
        UNDEF == p_feedback.ptab->close_proc (p_feedback.inst) ||
        UNDEF == sect_retrieve.ptab->close_proc (sect_retrieve.inst) ||
        UNDEF == close_get_q_user (get_q_user_inst) ||
        UNDEF == close_get_q_text (get_q_text_inst))
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
    if (UNDEF == save_vec ((VEC *) is->retrieval.query->vector))
        return (UNDEF);

    if (UNDEF == p_retrieve.ptab->proc (&is->retrieval,
                                        &results,
                                        p_retrieve.inst))
        return (UNDEF);
    if (UNDEF == res_trvec (&results, is->retrieval.tr))
        return (UNDEF);

    if (UNDEF == inter_prepare_titles (is, (char *) NULL))
        return (UNDEF);

    is->current_doc = 0;

    return (show_titles (is, (char *) NULL));

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

static VEC dvec;

int
run_dvec (is, unused)
INTER_STATE *is;
char *unused;
{
    long docid, paraid, sentid;
    int status;
    char temp_buf[PATH_LEN];

    if (is->num_command_line < 2 ||
        1 != inter_get_docid (is->command_line[1],
                              &docid, &paraid, &sentid) ||
        docid == 0) {
        if (UNDEF == add_buf_string ("Not a valid document. Command ignored\n",
                                     &is->err_buf))
            return (UNDEF);
        return (0);
    }

    is->retrieval.tr->num_tr = 0;

    if (paraid >= 0)
        return (run_pvec (is, docid, paraid, sentid));
    if (sentid >= 0)
        return (run_svec (is, docid, sentid));

    if (UNDEF == (status = did_vec (&docid, &dvec, did_vec_inst))) {
        return (UNDEF);
    }
    if (status == 0) {
        (void) sprintf (temp_buf,
                        "Doc %ld is not valid.  Command ignored\n",
                        docid);
        if (UNDEF == add_buf_string (temp_buf, &is->err_buf))
            return (UNDEF);
        return (0);
    }
    is->retrieval.query->vector = (char *) &dvec;

    return (run_retrieve (is));
}


static int
run_pvec (is, docid, paraid, sentid)
INTER_STATE *is;
long docid, paraid, sentid;
{
    return (run_sect_vec (is, docid, paraid, &para_pre, "Paragraph"));
}

static int
run_svec (is, docid, sentid)
INTER_STATE *is;
long docid, sentid;
{
    return (run_sect_vec (is, docid, sentid, &sent_pre, "Sentence"));
}


static int
run_sect_vec (is, docid, sectid, preparse, name)
INTER_STATE *is;
long docid, sectid;
PROC_INST *preparse;
char *name;
{
    SM_INDEX_TEXTDOC pp;
    char temp_buf[PATH_LEN];
    VEC_LIST vec_list;

    if (sectid == 0) {
        if (UNDEF == add_buf_string ("Can only run one section of doc\n",
                                     &is->err_buf))
            return (UNDEF);
        return (0);
    }

    if (UNDEF == inter_get_pp_sect (docid, sectid, preparse, &pp) ||
        UNDEF ==  pp_veclist (&pp, &vec_list, pp_veclist_inst) ||
        2 != vec_list.num_vec) {
        (void) sprintf (temp_buf,
                        "Doc %d %s %d cannot be indexed\n",
                        docid, name, sectid);
        if (UNDEF == add_buf_string (temp_buf, &is->err_buf))
            return (UNDEF);
        return (0);
    }

    /* Save a copy of vec_list so won't overwrite (NOTE MEMORY LEAK) */
    if (UNDEF == save_vec_list (&vec_list))
        return (UNDEF);
    is->retrieval.query->vector = (char *) &vec_list.vec[1];
    is->retrieval.tr->num_tr = 0;

    return (run_retrieve (is));
}


int
run_feedback(is, unused)
INTER_STATE *is;
char *unused;
{
    QUERY_VECTOR fdbk_query;
    TR_VEC *tr_vec;
    int command, i;
    long docid, paraid, sentid;
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
                                      &docid, &paraid, &sentid))
		return (UNDEF);

        for (i = 0;
             i < tr_vec->num_tr && docid != tr_vec->tr[i].did;
             i++)
            ;
        if (i >= tr_vec->num_tr) {
            (void) sprintf (temp_buf,
                            "Document %d cannot be used for feedback.", docid);
            if (UNDEF == add_buf_string (temp_buf, &is->err_buf))
                return (UNDEF);
            return (0);
        }
        tr_vec->tr[i].rel = 1;
}

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

    if (1 != (status = get_q_user (NULL,
                                   &is->query_textdisplay,
                                   get_q_user_inst))) {
        return (status);
    }
    if (1 != (status = get_q_text (&is->query_textdisplay,
                                   &query_vec,
                                   get_q_text_inst))) {
        return (status);
    }

    is->retrieval.tr->num_tr = 0;
    is->retrieval.query->vector = query_vec.vector;
    is->retrieval.query->qid = query_vec.qid;

    return (run_retrieve (is));
}


int
x_run_new (is, user_input)
INTER_STATE *is;
char *user_input;
{
    int status;
    QUERY_VECTOR query_vec;

    if (1 != (status = x_get_q_user (user_input,
                                   &is->query_textdisplay,
                                   get_q_user_inst))) {
        return (status);
    }

    if (1 != (status = get_q_text (&is->query_textdisplay,
                                   &query_vec,
                                   get_q_text_inst))) {
        return (status);
    }

    is->retrieval.tr->num_tr = 0;
    is->retrieval.query->vector = query_vec.vector;
    is->retrieval.query->qid = query_vec.qid;

    is->output_buf.end = 0;

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

