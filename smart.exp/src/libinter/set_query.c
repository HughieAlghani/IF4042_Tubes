#ifdef RCSID
static char rcsid[] = "$Header: /home/smart/release/src/libinter/set_query.c,v 11.0 1992/07/21 18:21:48 chrisb Exp $";
#endif

/* Copyright (c) 1991, 1990, 1984 - Gerard Salton, Chris Buckley. 

   Permission is granted for use of this file in unmodified form for
   research purposes. Please contact the SMART project to obtain 
   permission for other uses.
*/

#include "common.h"
#include "param.h"
#include "functions.h"
#include "spec.h"
#include "inter.h"
#include "textloc.h"
#include "sm_display.h"
#include "smart_error.h"

static char *tr_file;
static long tr_mode;
static char *textloc_file;
static long textloc_mode;

static SPEC_PARAM spec_args[] = {
    "retrieve.tr_file",           getspec_dbfile,   (char *) &tr_file,
    "retrieve.tr_file.rmode",     getspec_filemode, (char *) &tr_mode,
    "inter.query.textloc_file",      getspec_dbfile,   (char *) &textloc_file,
    "inter.query.textloc_file.rmode",getspec_filemode, (char *) &textloc_mode,
    };
static int num_spec_args = sizeof (spec_args) /
         sizeof (spec_args[0]);

static int num_init = 0;

static int tr_fd;
static TR_TUP *saved_tr_tup;
static int num_saved_tr_tup;
static int qid_vec_inst;
static int textloc_fd;
static int valid_info;

static VEC qvec;
static QUERY_VECTOR query_vector;

int
init_set_query (spec, unused)
SPEC *spec;
char *unused;
{
    /* Only initialize the first time this is called. */
    if (num_init++)
        return (0);

    if (UNDEF == lookup_spec (spec, &spec_args[0], num_spec_args))
        return (UNDEF);

    if (UNDEF == (qid_vec_inst = init_qid_vec (spec, (char *) NULL))) {
        clr_err();
        valid_info = 0;
        return (0);
    }
    tr_fd = UNDEF;
    if (VALID_FILE (tr_file))
        tr_fd = open_tr_vec (tr_file, tr_mode);

    /* Open query textloc file to get file location and offsets for qid */
    if (VALID_FILE (textloc_file)) {
        if (UNDEF == (textloc_fd = open_textloc (textloc_file, textloc_mode)))
            return (UNDEF);
    }
    else {
        valid_info = 0;
        return (0);
    }

    /* Reserve space for top-ranked list */
    num_saved_tr_tup = 20;
    if (NULL == (saved_tr_tup = (TR_TUP *)
                 malloc ((unsigned) num_saved_tr_tup * sizeof (TR_TUP))))
        return (UNDEF);

    valid_info = 1;

    return (0);
}


int
set_query (is, unused)
INTER_STATE *is;
char *unused;
{
    long qid;

    if (is->num_command_line < 2 || (0 == (qid = atol (is->command_line[1])))){
        if (UNDEF == add_buf_string ("No query specified\n", &is->err_buf))
            return (UNDEF);
        return (0);
    }
    if (valid_info == 0) {
        if (UNDEF == add_buf_string ("Query file not opened\n", &is->err_buf))
            return (UNDEF);
        return (0);
    }

    /* Get the top docs for this query */
    is->retrieval.tr->num_tr = 0;
    is->retrieval.tr->qid = qid;
    if (tr_fd != UNDEF &&
        1 == seek_tr_vec (tr_fd, is->retrieval.tr) &&
        1 == read_tr_vec (tr_fd, is->retrieval.tr)) {
        /* Must save a copy of the top docs in local memory */
        if (is->retrieval.tr->num_tr > num_saved_tr_tup) {
            num_saved_tr_tup = is->retrieval.tr->num_tr;
            if (NULL == (saved_tr_tup = (TR_TUP *)
                         realloc ((char *) saved_tr_tup,
                                  (unsigned) num_saved_tr_tup * 
                                  sizeof (TR_TUP))))
                return (UNDEF);
        }
        bcopy ((char *) is->retrieval.tr->tr,
               (char *) saved_tr_tup,
               (int) is->retrieval.tr->num_tr * sizeof (TR_TUP));
        is->retrieval.tr->tr = saved_tr_tup;
    }

    /* Prepare title listing of top docs */
    if (UNDEF == inter_prepare_titles (is, (char *) NULL))
        return (UNDEF);

    /* Get a description of the text of the query */
    is->query_textloc.id_num = qid;
    if (1 != seek_textloc (textloc_fd, &is->query_textloc) ||
        1 != read_textloc (textloc_fd, &is->query_textloc))
        is->query_textloc.id_num = UNDEF;

    /* Get actual query vector (from either pre-indexed vectors, or by
       re-indexing on the fly */
    if (1 != qid_vec (&qid, &qvec, qid_vec_inst)) {
        if (UNDEF == add_buf_string ("Can't find query specified\n",
                                     &is->err_buf))
            return (UNDEF);
        return (0);
    }
    query_vector.vector = (char *) &qvec;
    query_vector.qid = qid;
    is->retrieval.query = &query_vector;

    
    return (1);
}
        
int
close_set_query (inst)
int inst;
{
    /* Only close after all instantiations done */
    if (--num_init)
        return (0);

    if (valid_info == 0)
        return (0);

    if (UNDEF == close_qid_vec (qid_vec_inst))
        return (UNDEF);
    if (UNDEF != tr_fd && UNDEF == close_tr_vec (tr_fd))
        return (UNDEF);

    if (UNDEF != textloc_fd && UNDEF == close_textloc (textloc_fd))
        return (UNDEF);

    (void) free ((char *) saved_tr_tup);
    valid_info = 0;

    return (0);
}

